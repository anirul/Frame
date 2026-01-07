#include "frame/vulkan/device.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

#include <fstream>
#include <stdexcept>
#include <sstream>
#include <shaderc/shaderc.hpp>
#include "absl/flags/flag.h"

#include "frame/camera.h"
#include "frame/level.h"
#include "frame/common/application.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/raytracing_bindings.h"
#include "frame/vulkan/texture.h"
#include "frame/vulkan/texture_resources.h"
#include "frame/proto/uniform.pb.h"
#include <glm/gtc/packing.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace frame::vulkan
{

namespace
{

class ScopedTimer
{
  public:
    ScopedTimer(const Logger& logger, std::string label)
        : logger_(logger),
          label_(std::move(label)),
          start_(Clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - start_);
        logger_->info("{} took {} ms.", label_, elapsed.count());
    }

  private:
    using Clock = std::chrono::steady_clock;
    const Logger& logger_;
    std::string label_;
    Clock::time_point start_;
};

struct DebugVertex
{
    glm::vec3 position;
    float pad0;
    glm::vec3 normal;
    float pad1;
    glm::vec2 uv;
    glm::vec2 pad2;
};

struct DebugTriangle
{
    DebugVertex v0;
    DebugVertex v1;
    DebugVertex v2;
};


struct DebugBvhNode
{
    glm::vec4 min;
    glm::vec4 max;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

bool RayTriangleIntersectDebug(
    const glm::vec3& ray_origin,
    const glm::vec3& ray_direction,
    const DebugTriangle& triangle,
    float& out_t)
{
    constexpr float kEpsilon = 0.0000001f;
    const glm::vec3 edge1 =
        glm::vec3(triangle.v1.position) - glm::vec3(triangle.v0.position);
    const glm::vec3 edge2 =
        glm::vec3(triangle.v2.position) - glm::vec3(triangle.v0.position);
    const glm::vec3 h = glm::cross(ray_direction, edge2);
    const float a = glm::dot(edge1, h);
    if (a > -kEpsilon && a < kEpsilon)
    {
        return false;
    }
    const float f = 1.0f / a;
    const glm::vec3 s = ray_origin - glm::vec3(triangle.v0.position);
    const float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }
    const glm::vec3 q = glm::cross(s, edge1);
    const float v = f * glm::dot(ray_direction, q);
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }
    const float t = f * glm::dot(edge2, q);
    if (t > kEpsilon)
    {
        out_t = t;
        return true;
    }
    return false;
}

bool RayAabbIntersectDebug(
    const glm::vec3& ray_origin,
    const glm::vec3& inv_ray_dir,
    const DebugBvhNode& node)
{
    const glm::vec3 t0 = (glm::vec3(node.min) - ray_origin) * inv_ray_dir;
    const glm::vec3 t1 = (glm::vec3(node.max) - ray_origin) * inv_ray_dir;
    const glm::vec3 tmin = glm::min(t0, t1);
    const glm::vec3 tmax = glm::max(t0, t1);
    const float t_enter = std::max(std::max(tmin.x, tmin.y), tmin.z);
    const float t_exit = std::min(std::min(tmax.x, tmax.y), tmax.z);
    return t_exit >= std::max(t_enter, 0.0f);
}

bool TraverseBvhDebug(
    const std::vector<DebugBvhNode>& nodes,
    const std::vector<DebugTriangle>& tris,
    const glm::vec3& ray_origin,
    const glm::vec3& ray_dir,
    float& out_t,
    int& out_tri)
{
    if (nodes.empty())
    {
        return false;
    }

    const glm::vec3 inv_ray_dir = 1.0f / ray_dir;
    int stack[64];
    int sp = 0;
    stack[sp++] = 0;
    out_t = std::numeric_limits<float>::max();
    out_tri = -1;
    bool hit = false;
    while (sp > 0)
    {
        const int node_index = stack[--sp];
        const auto& node = nodes[static_cast<std::size_t>(node_index)];
        if (!RayAabbIntersectDebug(ray_origin, inv_ray_dir, node))
        {
            continue;
        }
        if (node.triangle_count > 0)
        {
            for (int i = 0; i < node.triangle_count; ++i)
            {
                const int tri_index = node.first_triangle + i;
                if (tri_index < 0 ||
                    static_cast<std::size_t>(tri_index) >= tris.size())
                {
                    continue;
                }
                float t = 0.0f;
                if (RayTriangleIntersectDebug(
                        ray_origin, ray_dir, tris[static_cast<std::size_t>(tri_index)], t) &&
                    t < out_t)
                {
                    out_t = t;
                    out_tri = tri_index;
                    hit = true;
                }
            }
        }
        else
        {
            if (node.left >= 0)
            {
                stack[sp++] = node.left;
            }
            if (node.right >= 0)
            {
                stack[sp++] = node.right;
            }
        }
    }
    return hit;
}

} // namespace

Device::Device(
    void* vk_instance,
    glm::uvec2 size,
    vk::SurfaceKHR& surface)
    : vk_instance_(static_cast<VkInstance>(vk_instance)),
      size_(size),
      vk_surface_(surface),
      texture_resources_(std::make_unique<TextureResources>(*this))
{
    debug_dump_compute_output_ = absl::GetFlag(FLAGS_vk_dump_compute);
    debug_log_scene_state_ = absl::GetFlag(FLAGS_vk_log_scene);
    debug_hit_mask_ = absl::GetFlag(FLAGS_vk_debug_hitmask);

    logger_->info("Initializing Vulkan device ({}x{})", size_.x, size_.y);

    std::vector<vk::PhysicalDevice> physical_devices =
        vk_instance_.enumeratePhysicalDevices();
    if (physical_devices.empty())
    {
        throw std::runtime_error("No Vulkan physical device found.");
    }

    int best_score = std::numeric_limits<int>::min();
    for (const auto& physical_device : physical_devices)
    {
        const auto properties = physical_device.getProperties();
        const auto features = physical_device.getFeatures();

        int score = 0;
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            score += 1000;
        }
        score += static_cast<int>(properties.limits.maxImageDimension2D);
        if (!features.geometryShader)
        {
            continue;
        }

        const std::string device_name(properties.deviceName.data());
        logger_->info("Evaluated Vulkan device: {}", device_name);
        if (score > best_score)
        {
            best_score = score;
            vk_physical_device_ = physical_device;
        }
    }

    if (!vk_physical_device_)
    {
        throw std::runtime_error(
            "No suitable Vulkan physical device with geometry shader support.");
    }

    const auto queue_families = vk_physical_device_.getQueueFamilyProperties();
    std::optional<std::uint32_t> graphics_queue_index;
    std::optional<std::uint32_t> present_queue_index;

    for (std::uint32_t index = 0; index < queue_families.size(); ++index)
    {
        const auto& queue_family = queue_families[index];

        const bool supports_graphics =
            (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) ==
            vk::QueueFlagBits::eGraphics;
        if (supports_graphics && !graphics_queue_index)
        {
            graphics_queue_index = index;
        }

        const bool supports_present =
            vk_physical_device_.getSurfaceSupportKHR(index, vk_surface_);
        if (supports_present && !present_queue_index)
        {
            present_queue_index = index;
        }

        if (graphics_queue_index && present_queue_index)
        {
            break;
        }
    }

    if (!graphics_queue_index)
    {
        throw std::runtime_error("No Vulkan queue family supporting graphics.");
    }

    if (!present_queue_index)
    {
        if (vk_physical_device_.getSurfaceSupportKHR(
                graphics_queue_index.value(),
                vk_surface_))
        {
            present_queue_index = graphics_queue_index;
        }
        else
        {
            throw std::runtime_error(
                "No Vulkan queue family supporting presentation.");
        }
    }

    graphics_queue_family_index_ = graphics_queue_index.value();
    present_queue_family_index_ = present_queue_index.value();

    std::vector<std::uint32_t> unique_queue_indices = {graphics_queue_family_index_};
    if (present_queue_family_index_ != graphics_queue_family_index_)
    {
        unique_queue_indices.push_back(present_queue_family_index_);
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(unique_queue_indices.size());
    for (auto index : unique_queue_indices)
    {
        queue_create_infos.emplace_back(
            vk::DeviceQueueCreateFlags{},
            index,
            1,
            &queue_family_priority_);
    }

    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    const auto supported_features = vk_physical_device_.getFeatures();
    vk::PhysicalDeviceFeatures device_features{};
    device_features.geometryShader = supported_features.geometryShader;
    device_features.samplerAnisotropy = supported_features.samplerAnisotropy;
    device_features.shaderStorageImageExtendedFormats =
        supported_features.shaderStorageImageExtendedFormats;
    if (!device_features.shaderStorageImageExtendedFormats &&
        compute_output_format_ == vk::Format::eR16G16B16A16Sfloat)
    {
        logger_->warn(
            "shaderStorageImageExtendedFormats not supported; "
            "raytracing compute output uses rgba16f storage images and may fail.");
    }

    vk::DeviceCreateInfo device_create_info(
        {},
        static_cast<std::uint32_t>(queue_create_infos.size()),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<std::uint32_t>(device_extensions.size()),
        device_extensions.data());
    device_create_info.setPEnabledFeatures(&device_features);

    vk_unique_device_ = vk_physical_device_.createDeviceUnique(device_create_info);
    graphics_queue_ = vk_unique_device_->getQueue(graphics_queue_family_index_, 0);
    present_queue_ = vk_unique_device_->getQueue(present_queue_family_index_, 0);

    logger_->info(
        "Vulkan logical device created (graphics queue family {}, present queue family {}).",
        graphics_queue_family_index_,
        present_queue_family_index_);
}

Device::~Device()
{
    Shutdown();
}

void Device::SetStereo(
    StereoEnum stereo_enum,
    float interocular_distance,
    glm::vec3 focus_point,
    bool invert_left_right)
{
    stereo_enum_ = stereo_enum;
    interocular_distance_ = interocular_distance;
    focus_point_ = focus_point;
    invert_left_right_ = invert_left_right;
}

void Device::Clear(const glm::vec4& /*color*/) const
{
    // TODO: hook up Vulkan render passes once the swapchain exists.
}

void Device::Startup(std::unique_ptr<LevelInterface>&& level)
{
    level_ = std::move(level);
}

void Device::StartupFromLevelData(const frame::json::LevelData& level_data)
{
    ScopedTimer total_timer(logger_, "Vulkan StartupFromLevelData");

    current_level_data_ = level_data;
    active_program_info_.reset();
    use_procedural_quad_pipeline_ = false;
    use_compute_raytracing_ = false;
    compute_output_in_shader_read_ = false;
    elapsed_time_seconds_ = 0.0f;

    // Prefer the raytracing program (QUAD) if present; otherwise fall back to
    // the first available program.
    {
        ScopedTimer timer(logger_, "Select program");
        auto pick_program = [&]() -> std::optional<frame::json::ProgramInfo> {
            for (const auto& program : level_data.programs)
            {
                if (program.vertex_shader == "raytracing.vert" ||
                    program.name == "RayTraceProgram")
                {
                    return program;
                }
            }
            if (!level_data.programs.empty())
            {
                return level_data.programs.front();
            }
            return std::nullopt;
        };

        if (auto chosen_program = pick_program())
        {
            ProgramPipelineInfo pipeline_info;
            const auto& program_info = *chosen_program;
            pipeline_info.program_name = program_info.name;
            const auto shader_root =
                level_data.asset_root / "shader" / "vulkan";
            pipeline_info.vertex_shader = shader_root / program_info.vertex_shader;
            pipeline_info.fragment_shader =
                shader_root / program_info.fragment_shader;
            pipeline_info.use_compute =
                (program_info.name == "RayTraceProgram");
            if (pipeline_info.use_compute)
            {
                bool wants_dual_buffers = false;
                for (const auto& proto_mat : level_data.proto.materials())
                {
                    if (proto_mat.program_name() != program_info.name)
                    {
                        continue;
                    }
                    for (const auto& inner_name :
                         proto_mat.inner_buffer_names())
                    {
                        if (inner_name == "TriangleBufferGround")
                        {
                            wants_dual_buffers = true;
                            break;
                        }
                    }
                    if (wants_dual_buffers)
                    {
                        break;
                    }
                }
                const char* compute_name = wants_dual_buffers
                    ? "raytracing_dual.comp"
                    : "raytracing.comp";
                pipeline_info.compute_shader = shader_root / compute_name;
            }
            pipeline_info.scene_type = frame::proto::SceneType::NONE;
            for (const auto& proto_program : level_data.proto.programs())
            {
                if (proto_program.name() == program_info.name)
                {
                    pipeline_info.scene_type =
                        proto_program.input_scene_type().value();
                    for (const auto& uniform : proto_program.uniforms())
                    {
                        if (uniform.value_oneof_case() ==
                                frame::proto::Uniform::kUniformEnum &&
                            uniform.uniform_enum() ==
                                frame::proto::Uniform::FLOAT_TIME_S)
                        {
                            pipeline_info.uses_time_uniform = true;
                        }
                    }
                    break;
                }
            }
            active_program_info_ = std::move(pipeline_info);
        }
    }

    {
        ScopedTimer timer(logger_, "BuildLevel");
        auto built = BuildLevel(GetSize(), level_data);
        level_ = std::move(built.level);
    }

    if (active_program_info_ && level_)
    {
        active_program_info_->program_id =
            level_->GetIdFromName(active_program_info_->program_name);
        const bool allow_compute = absl::GetFlag(FLAGS_vk_raytrace_compute);
        use_compute_raytracing_ =
            active_program_info_->use_compute && allow_compute;
        active_program_info_->input_texture_ids.clear();
        active_program_info_->input_buffer_ids.clear();
        active_program_info_->input_buffer_inner_names.clear();
        if (active_program_info_->program_id != NullId)
        {
            try
            {
                auto& program = level_->GetProgramFromId(
                    active_program_info_->program_id);
                active_program_info_->input_texture_ids =
                    program.GetInputTextureIds();
                // Select a material bound to this program as the active one and
                // capture buffers in the same order as the proto definition.
                const frame::proto::Material* proto_material_ptr = nullptr;
                for (const auto& proto_mat : current_level_data_->proto.materials())
                {
                    if (proto_mat.program_name() ==
                        active_program_info_->program_name)
                    {
                        proto_material_ptr = &proto_mat;
                        break;
                    }
                }
                for (auto material_id : level_->GetMaterials())
                {
                    auto& material = level_->GetMaterialFromId(material_id);
                    if (material.GetProgramId(level_.get()) ==
                        active_program_info_->program_id)
                    {
                        active_program_info_->material_id = material_id;
                        auto material_texture_ids = material.GetTextureIds();
                        if (!material_texture_ids.empty())
                        {
                            auto program_texture_ids =
                                active_program_info_->input_texture_ids;
                            if (material_texture_ids.size() ==
                                program_texture_ids.size())
                            {
                                auto sort_ids =
                                    [](std::vector<EntityId>& ids) {
                                        std::sort(ids.begin(), ids.end());
                                    };
                                sort_ids(material_texture_ids);
                                sort_ids(program_texture_ids);
                                if (material_texture_ids == program_texture_ids)
                                {
                                    active_program_info_->input_texture_ids =
                                        material.GetTextureIds();
                                }
                                else
                                {
                                    logger_->warn(
                                        "Program/material texture mismatch for {} (keeping program order).",
                                        active_program_info_->program_name);
                                }
                            }
                            else
                            {
                                logger_->warn(
                                    "Program/material texture count mismatch for {} ({} vs {}); keeping program order.",
                                    active_program_info_->program_name,
                                    program_texture_ids.size(),
                                    material_texture_ids.size());
                            }
                        }
                        if (proto_material_ptr)
                        {
                            for (int i = 0;
                                 i < proto_material_ptr->buffer_names_size();
                                 ++i)
                            {
                                const auto& buffer_name =
                                    proto_material_ptr->buffer_names(i);
                                auto buffer_id =
                                    level_->GetIdFromName(buffer_name);
                                if (buffer_id != NullId)
                                {
                                    active_program_info_->input_buffer_ids.push_back(
                                        buffer_id);
                                    active_program_info_
                                        ->input_buffer_inner_names.push_back(
                                            proto_material_ptr
                                                ->inner_buffer_names(i));
                                }
                            }
                        }
                        // Ensure triangle/BVH SSBOs are captured even if the
                        // proto material list is missing or incomplete.
                        auto append_buffer_if_present =
                            [&](EntityId buffer_id,
                                const std::string& inner_name) {
                                if (buffer_id == NullId)
                                {
                                    return;
                                }
                                if (std::find(
                                        active_program_info_
                                            ->input_buffer_ids.begin(),
                                        active_program_info_
                                            ->input_buffer_ids.end(),
                                        buffer_id) ==
                                    active_program_info_
                                        ->input_buffer_ids.end())
                                {
                                    active_program_info_->input_buffer_ids.push_back(
                                        buffer_id);
                                    // Keep binding order stable by mirroring
                                    // the shader names.
                                    active_program_info_
                                        ->input_buffer_inner_names.push_back(
                                            inner_name);
                                }
                            };
                        // Collect buffers from all meshes using this material.
                        const std::array<
                            frame::proto::NodeStaticMesh::RenderTimeEnum, 2>
                            render_times = {
                                frame::proto::NodeStaticMesh::PRE_RENDER_TIME,
                                frame::proto::NodeStaticMesh::SCENE_RENDER_TIME};
                        for (auto render_time : render_times)
                        {
                            const auto mesh_pairs =
                                level_->GetStaticMeshMaterialIds(render_time);
                            for (const auto& pair : mesh_pairs)
                            {
                                if (pair.second != material_id)
                                {
                                    continue;
                                }
                                auto& node =
                                    level_->GetSceneNodeFromId(pair.first);
                                const auto mesh_id = node.GetLocalMesh();
                                if (mesh_id == NullId)
                                {
                                    continue;
                                }
                                auto& mesh =
                                    level_->GetStaticMeshFromId(mesh_id);
                                append_buffer_if_present(
                                    mesh.GetTriangleBufferId(),
                                    "TriangleBuffer");
                                append_buffer_if_present(
                                    mesh.GetBvhBufferId(), "BvhBuffer");
                            }
                        }
                        break;
                    }
                }
            }
            catch (const std::exception& ex)
            {
                logger_->warn(
                    "Failed to gather texture bindings for program {}: {}",
                    active_program_info_->program_name,
                    ex.what());
            }
        }
    }

    if (!command_pool_)
    {
        CreateCommandPool();
    }

    DestroyComputePipeline();
    DestroyDescriptorResources();
    DestroyTextureResources();
    if (mesh_resources_)
    {
        mesh_resources_->Clear();
    }

    try
    {
        {
            ScopedTimer timer(logger_, "CreateTextureResources");
            CreateTextureResources(level_data);
        }
        DestroySwapchainResources();
        {
            ScopedTimer timer(logger_, "CreateSwapchainResources");
            CreateSwapchainResources();
        }
        {
            ScopedTimer timer(logger_, "CreateDescriptorResources");
            CreateDescriptorResources();
        }
        if (mesh_resources_)
        {
            ScopedTimer timer(logger_, "MeshResources Build");
            mesh_resources_->Build(level_data);
        }
        {
            ScopedTimer timer(logger_, "CreateGraphicsPipeline");
            CreateGraphicsPipeline();
        }
        if (use_compute_raytracing_)
        {
            ScopedTimer timer(logger_, "CreateComputePipeline");
            CreateComputePipeline();
        }
    }
    catch (const std::exception& ex)
    {
        logger_->error("Failed to prepare Vulkan GPU resources: {}", ex.what());
        DestroyDescriptorResources();
        DestroyTextureResources();
        if (mesh_resources_)
        {
            mesh_resources_->Clear();
        }
        DestroyGraphicsPipeline();
        DestroyComputePipeline();
    }

    if (!sync_objects_created_)
    {
        CreateSyncObjects();
        sync_objects_created_ = true;
    }
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface)
{
    if (!plugin_interface)
    {
        return;
    }
    plugin_interfaces_.push_back(std::move(plugin_interface));
}

std::vector<PluginInterface*> Device::GetPluginPtrs()
{
    std::vector<PluginInterface*> plugins;
    plugins.reserve(plugin_interfaces_.size());
    for (auto& plugin : plugin_interfaces_)
    {
        plugins.push_back(plugin.get());
    }
    return plugins;
}

std::vector<std::string> Device::GetPluginNames() const
{
    std::vector<std::string> names;
    names.reserve(plugin_interfaces_.size());
    for (const auto& plugin : plugin_interfaces_)
    {
        names.push_back(plugin->GetName());
    }
    return names;
}

void Device::RemovePluginByName(const std::string& name)
{
    std::erase_if(plugin_interfaces_, [&name](const auto& plugin) {
        return plugin && plugin->GetName() == name;
    });
}

void Device::Cleanup()
{
    if (vk_unique_device_)
    {
        const VkResult result =
            vkDeviceWaitIdle(static_cast<VkDevice>(*vk_unique_device_));
        if (result != VK_SUCCESS)
        {
            logger_->warn(
                "vkDeviceWaitIdle failed during cleanup: {}",
                vk::to_string(static_cast<vk::Result>(result)));
        }
    }

    DestroyGraphicsPipeline();
    DestroySwapchainResources();
    DestroyDescriptorResources();
    DestroyTextureResources();
    if (mesh_resources_)
    {
        mesh_resources_->Clear();
    }
    command_pool_.reset();

    for (auto& semaphore : image_available_semaphores_)
    {
        semaphore.reset();
    }
    for (auto& semaphore : render_finished_semaphores_)
    {
        semaphore.reset();
    }
    for (auto& fence : in_flight_fences_)
    {
        fence.reset();
    }

    sync_objects_created_ = false;
    current_level_data_.reset();

    for (auto& plugin : plugin_interfaces_)
    {
        if (plugin)
        {
            plugin->End();
        }
    }
    plugin_interfaces_.clear();
    level_.reset();
    elapsed_time_seconds_ = 0.0f;
    active_program_info_.reset();
    use_procedural_quad_pipeline_ = false;
    push_constant_stages_ = {};
    push_constant_size_ = 0;
}

void Device::Resize(glm::uvec2 size)
{
    size_ = size;
    framebuffer_resized_ = true;
    for (auto& plugin : plugin_interfaces_)
    {
        if (plugin)
        {
            plugin->Startup(size_);
        }
    }
}

glm::uvec2 Device::GetSize() const
{
    return size_;
}

void Device::Display(double dt)
{
    if (device_lost_)
    {
        return;
    }

    elapsed_time_seconds_ += static_cast<float>(dt);

    if (level_)
    {
        level_->UpdateLights(static_cast<double>(elapsed_time_seconds_));
    }

    if (!vk_unique_device_ || !swapchain_)
    {
        return;
    }
    if (!graphics_pipeline_ || !pipeline_layout_)
    {
        return;
    }

    if (framebuffer_resized_)
    {
        framebuffer_resized_ = false;
        RecreateSwapchain();
        return;
    }

    const auto& fence = in_flight_fences_[current_frame_];
    const VkFence fence_handle = static_cast<VkFence>(*fence);
    const VkResult wait_result = vkWaitForFences(
        static_cast<VkDevice>(*vk_unique_device_),
        1,
        &fence_handle,
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());
    if (wait_result != VK_SUCCESS)
    {
        logger_->error(
            "vkWaitForFences failed: {}",
            vk::to_string(static_cast<vk::Result>(wait_result)));
        if (wait_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    auto acquire = vk_unique_device_->acquireNextImageKHR(
        *swapchain_,
        std::numeric_limits<std::uint64_t>::max(),
        *image_available_semaphores_[current_frame_],
        nullptr);

    if (acquire.result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapchain();
        return;
    }
    if (acquire.result != vk::Result::eSuccess &&
        acquire.result != vk::Result::eSuboptimalKHR)
    {
        logger_->error(
            "Failed to acquire swapchain image: {}",
            vk::to_string(acquire.result));
        if (acquire.result == vk::Result::eErrorDeviceLost)
        {
            device_lost_ = true;
        }
        return;
    }

    const std::uint32_t image_index = acquire.value;
    const VkResult reset_result = vkResetFences(
        static_cast<VkDevice>(*vk_unique_device_),
        1,
        &fence_handle);
    if (reset_result != VK_SUCCESS)
    {
        logger_->error(
            "vkResetFences failed: {}",
            vk::to_string(static_cast<vk::Result>(reset_result)));
        if (reset_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    vk::CommandBuffer command_buffer = command_buffers_[current_frame_];
    command_buffer.reset();
    RecordCommandBuffer(command_buffer, image_index);

    const vk::Semaphore wait_semaphores[] = {
        *image_available_semaphores_[current_frame_]};
    const vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::Semaphore signal_semaphores[] = {
        *render_finished_semaphores_[current_frame_]};

    vk::SubmitInfo submit_info(
        1,
        wait_semaphores,
        wait_stages,
        1,
        &command_buffer,
        1,
        signal_semaphores);

    const VkSubmitInfo submit_info_c = submit_info;
    const VkResult submit_result = vkQueueSubmit(
        static_cast<VkQueue>(graphics_queue_),
        1,
        &submit_info_c,
        *fence);
    if (submit_result != VK_SUCCESS)
    {
        logger_->error(
            "vkQueueSubmit failed: {}",
            vk::to_string(static_cast<vk::Result>(submit_result)));
        if (submit_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    if ((debug_dump_compute_output_ || !debug_dump_done_) &&
        debug_readback_.buffer &&
        !debug_dump_done_)
    {
        const VkResult wait_result =
            vkQueueWaitIdle(static_cast<VkQueue>(graphics_queue_));
        if (wait_result != VK_SUCCESS)
        {
            logger_->warn(
                "vkQueueWaitIdle failed for debug readback: {}",
                vk::to_string(static_cast<vk::Result>(wait_result)));
            debug_dump_done_ = true;
        }
        else
        {
            void* mapped = vk_unique_device_->mapMemory(
                *debug_readback_.memory, 0, debug_readback_.size);
            if (mapped)
            {
                const auto* raw16 =
                    static_cast<const std::uint16_t*>(mapped);
                auto sample_half2 = [](const std::uint16_t* p) -> glm::vec2 {
                    return glm::unpackHalf2x16(
                        (static_cast<std::uint32_t>(p[1]) << 16) | p[0]);
                };
                if (debug_readback_.size == sizeof(std::uint16_t) * 4)
                {
                    const glm::vec2 rg = sample_half2(raw16);
                    const glm::vec2 ba = sample_half2(raw16 + 2);
                    logger_->info(
                        "Compute output first pixel: ({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                        rg.x,
                        rg.y,
                        ba.x,
                        ba.y);
                }
                else
                {
                    const std::size_t pixel_count =
                        swapchain_extent_.width * swapchain_extent_.height;
                    double sum_r = 0.0;
                    double sum_g = 0.0;
                    double sum_b = 0.0;
                    for (std::size_t i = 0; i < pixel_count; ++i)
                    {
                        const std::uint16_t* px = raw16 + i * 4;
                        const glm::vec2 rg = sample_half2(px);
                        const glm::vec2 ba = sample_half2(px + 2);
                        sum_r += rg.x;
                        sum_g += rg.y;
                        sum_b += ba.x;
                    }
                    if (pixel_count > 0)
                    {
                        const float avg_r =
                            static_cast<float>(sum_r / pixel_count);
                        const float avg_g =
                            static_cast<float>(sum_g / pixel_count);
                        const float avg_b =
                            static_cast<float>(sum_b / pixel_count);
                        if (!std::isfinite(avg_r) || !std::isfinite(avg_g) ||
                            !std::isfinite(avg_b))
                        {
                            logger_->warn("Debug compute capture avg color is NaN/Inf.");
                        }
                        else
                        {
                            logger_->info(
                                "Debug compute capture avg color: ({:.3f}, {:.3f}, {:.3f})",
                                avg_r,
                                avg_g,
                                avg_b);
                        }
                    }
                }
                const std::size_t cx = swapchain_extent_.width / 2;
                const std::size_t cy = swapchain_extent_.height / 2;
                const std::size_t center_index =
                    (cy * swapchain_extent_.width + cx) * 4;
                const auto* center_px = raw16 + center_index;
                const glm::vec2 center_rg = sample_half2(center_px);
                const glm::vec2 center_ba = sample_half2(center_px + 2);
                logger_->info(
                    "Center pixel color (compute output): "
                    "({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                    center_rg.x,
                    center_rg.y,
                    center_ba.x,
                    center_ba.y);
            }
            vk_unique_device_->unmapMemory(*debug_readback_.memory);
        }
        debug_dump_done_ = true;
    }

    vk::PresentInfoKHR present_info(
        1,
        signal_semaphores,
        1,
        &swapchain_.get(),
        &image_index);

    const vk::Result present_result = present_queue_.presentKHR(present_info);
    if (present_result == vk::Result::eErrorOutOfDateKHR ||
        present_result == vk::Result::eSuboptimalKHR)
    {
        RecreateSwapchain();
    }
    else if (present_result != vk::Result::eSuccess)
    {
        logger_->error(
            "Failed to present swapchain image: {}",
            vk::to_string(present_result));
        if (present_result == vk::Result::eErrorDeviceLost)
        {
            device_lost_ = true;
        }
        return;
    }

    if (absl::GetFlag(FLAGS_vk_validation))
    {
        const VkResult present_wait = vkQueueWaitIdle(
            static_cast<VkQueue>(present_queue_));
        if (present_wait != VK_SUCCESS)
        {
            logger_->error(
                "vkQueueWaitIdle after present failed: {}",
                vk::to_string(static_cast<vk::Result>(present_wait)));
            if (present_wait == VK_ERROR_DEVICE_LOST)
            {
                device_lost_ = true;
            }
            return;
        }
    }

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}


void Device::Shutdown()
{
    Cleanup();
    vk_unique_device_.reset();
}

void Device::ScreenShot(const std::string& file) const
{
    logger_->warn("Vulkan screenshot not implemented (requested: {})", file);
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(
    std::vector<float>&& /*vector*/)
{
    throw std::runtime_error("Vulkan point buffer support not implemented yet.");
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& /*vector*/)
{
    throw std::runtime_error("Vulkan index buffer support not implemented yet.");
}

std::unique_ptr<frame::StaticMeshInterface> Device::CreateStaticMesh(
    const StaticMeshParameter& /*static_mesh_parameter*/)
{
    throw std::runtime_error("Vulkan static mesh support not implemented yet.");
}

void Device::CreateCommandPool()
{
    vk::CommandPoolCreateInfo pool_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        graphics_queue_family_index_);
    command_pool_ = vk_unique_device_->createCommandPoolUnique(pool_info);
    gpu_memory_manager_ = std::make_unique<GpuMemoryManager>(
        vk_physical_device_, *vk_unique_device_);
    command_queue_ = std::make_unique<CommandQueue>(
        *vk_unique_device_, graphics_queue_, *command_pool_);
    buffer_resources_ = std::make_unique<BufferResourceManager>(
        *vk_unique_device_,
        *gpu_memory_manager_,
        *command_queue_,
        logger_);
    mesh_resources_ = std::make_unique<MeshResources>(
        *vk_unique_device_,
        *gpu_memory_manager_,
        *command_queue_,
        logger_);
}

void Device::CreateSyncObjects()
{
    for (std::size_t i = 0; i < kMaxFramesInFlight; ++i)
    {
        image_available_semaphores_[i] =
            vk_unique_device_->createSemaphoreUnique({});
        render_finished_semaphores_[i] =
            vk_unique_device_->createSemaphoreUnique({});
        vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
        in_flight_fences_[i] =
            vk_unique_device_->createFenceUnique(fence_info);
    }
}

vk::SurfaceFormatKHR Device::SelectSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& formats) const
{
    for (const auto& format : formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    for (const auto& format : formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    return formats.front();
}

vk::PresentModeKHR Device::SelectPresentMode(
    const std::vector<vk::PresentModeKHR>& modes) const
{
    for (const auto& mode : modes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Device::SelectSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    vk::Extent2D actual_extent{
        static_cast<std::uint32_t>(size_.x),
        static_cast<std::uint32_t>(size_.y)};

    actual_extent.width = std::clamp(
        actual_extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(
        actual_extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actual_extent;
}

void Device::CreateSwapchainResources()
{
    const auto capabilities =
        vk_physical_device_.getSurfaceCapabilitiesKHR(vk_surface_);
    const auto formats =
        vk_physical_device_.getSurfaceFormatsKHR(vk_surface_);
    const auto present_modes =
        vk_physical_device_.getSurfacePresentModesKHR(vk_surface_);

    const vk::SurfaceFormatKHR surface_format =
        SelectSurfaceFormat(formats);
    const vk::PresentModeKHR present_mode =
        SelectPresentMode(present_modes);
    const vk::Extent2D extent = SelectSwapExtent(capabilities);

    std::uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_info(
        {},
        vk_surface_,
        image_count,
        surface_format.format,
        surface_format.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment);

    std::array<std::uint32_t, 2> queue_family_indices = {
        graphics_queue_family_index_,
        present_queue_family_index_};
    if (graphics_queue_family_index_ != present_queue_family_index_)
    {
        swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
    {
        swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }

    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;

    swapchain_ = vk_unique_device_->createSwapchainKHRUnique(swapchain_info);
    swapchain_images_ = vk_unique_device_->getSwapchainImagesKHR(*swapchain_);
    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;

    swapchain_image_views_.clear();
    swapchain_image_views_.reserve(swapchain_images_.size());
    for (const auto& image : swapchain_images_)
    {
        vk::ImageViewCreateInfo view_info(
            {},
            image,
            vk::ImageViewType::e2D,
            swapchain_image_format_,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        swapchain_image_views_.push_back(
            vk_unique_device_->createImageViewUnique(view_info));
    }

    vk::AttachmentDescription color_attachment(
        {},
        swapchain_image_format_,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_ref(
        0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &color_ref,
        nullptr,
        nullptr,
        0,
        nullptr);

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo render_pass_info(
        {},
        1,
        &color_attachment,
        1,
        &subpass,
        1,
        &dependency);

    render_pass_ = vk_unique_device_->createRenderPassUnique(render_pass_info);

    framebuffers_.clear();
    framebuffers_.reserve(swapchain_image_views_.size());
    for (const auto& view : swapchain_image_views_)
    {
        vk::FramebufferCreateInfo framebuffer_info(
            {},
            *render_pass_,
            1,
            &view.get(),
            swapchain_extent_.width,
            swapchain_extent_.height,
            1);
        framebuffers_.push_back(
            vk_unique_device_->createFramebufferUnique(framebuffer_info));
    }

    if (!command_buffers_.empty())
    {
        vk_unique_device_->freeCommandBuffers(
            *command_pool_, command_buffers_);
        command_buffers_.clear();
    }

    vk::CommandBufferAllocateInfo allocate_info(
        *command_pool_,
        vk::CommandBufferLevel::ePrimary,
        static_cast<std::uint32_t>(kMaxFramesInFlight));
    command_buffers_ =
        vk_unique_device_->allocateCommandBuffers(allocate_info);
}

void Device::DestroySwapchainResources()
{
    DestroyComputePipeline();
    DestroyGraphicsPipeline();

    if (vk_unique_device_ && command_pool_ && !command_buffers_.empty())
    {
        vk_unique_device_->freeCommandBuffers(
            *command_pool_, command_buffers_);
        command_buffers_.clear();
    }
    framebuffers_.clear();
    render_pass_.reset();
    swapchain_image_views_.clear();
    swapchain_images_.clear();
    swapchain_.reset();
}

void Device::RecreateSwapchain()
{
    if (size_.x == 0 || size_.y == 0)
    {
        return;
    }

    if (vk_unique_device_)
    {
        vk_unique_device_->waitIdle();
    }

    DestroyComputePipeline();
    DestroyDescriptorResources();
    DestroySwapchainResources();
    CreateSwapchainResources();
    CreateDescriptorResources();
    CreateGraphicsPipeline();
    if (use_compute_raytracing_)
    {
        CreateComputePipeline();
    }
}

void Device::RecordCommandBuffer(
    vk::CommandBuffer command_buffer,
    std::uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info;
    command_buffer.begin(begin_info);

    const SceneState scene_state =
        (level_)
            ? BuildSceneState(
                  *level_,
                  frame::Logger::GetInstance(),
                  {swapchain_extent_.width, swapchain_extent_.height},
                  elapsed_time_seconds_,
                  active_program_info_
                      ? active_program_info_->material_id
                      : NullId,
                  !use_compute_raytracing_)
            : SceneState{};

    if (debug_log_scene_state_ && !debug_dump_done_)
    {
        logger_->info(
            "SceneState: proj[1][1]={:.3f}, view[3]={:.3f},{:.3f},{:.3f},{:.3f}, "
            "model[3]={:.3f},{:.3f},{:.3f},{:.3f}, camera=({:.3f},{:.3f},{:.3f})",
            scene_state.projection[1][1],
            scene_state.view[3][0],
            scene_state.view[3][1],
            scene_state.view[3][2],
            scene_state.view[3][3],
            scene_state.model[3][0],
            scene_state.model[3][1],
            scene_state.model[3][2],
            scene_state.model[3][3],
            scene_state.camera_position.x,
            scene_state.camera_position.y,
            scene_state.camera_position.z);
        logger_->info(
            "SceneState model matrix first row: {:.3f} {:.3f} {:.3f} {:.3f}",
            scene_state.model[0][0],
            scene_state.model[0][1],
            scene_state.model[0][2],
            scene_state.model[0][3]);

        // CPU-side sanity check: cast a center ray through the BVH using the
        // same data the compute shader consumes.
        if (level_)
        {
            EntityId tri_id = NullId;
            EntityId bvh_id = NullId;
            if (active_program_info_)
            {
                for (std::size_t i = 0;
                     i < active_program_info_->input_buffer_ids.size() &&
                     i < active_program_info_->input_buffer_inner_names.size();
                     ++i)
                {
                    const auto& inner =
                        active_program_info_->input_buffer_inner_names[i];
                    if (inner == "TriangleBuffer" ||
                        inner == "TriangleBufferGlass")
                    {
                        tri_id = active_program_info_->input_buffer_ids[i];
                    }
                    else if (inner == "TriangleBufferGround" &&
                             tri_id == NullId)
                    {
                        tri_id = active_program_info_->input_buffer_ids[i];
                    }
                    else if (inner == "BvhBuffer")
                    {
                        bvh_id = active_program_info_->input_buffer_ids[i];
                    }
                }
            }
            logger_->info(
                "CPU debug raytrace buffers tri_id={} bvh_id={}",
                static_cast<std::int64_t>(tri_id),
                static_cast<std::int64_t>(bvh_id));

            if (tri_id != NullId && bvh_id != NullId &&
                swapchain_extent_.width > 0 &&
                swapchain_extent_.height > 0)
            {
                try
                {
                    const auto& tri_buf =
                        dynamic_cast<const frame::vulkan::Buffer&>(
                            level_->GetBufferFromId(tri_id));
                    const auto& bvh_buf =
                        dynamic_cast<const frame::vulkan::Buffer&>(
                            level_->GetBufferFromId(bvh_id));
                    const auto& tri_bytes = tri_buf.GetRawData();
                    const auto& bvh_bytes = bvh_buf.GetRawData();
                    const std::size_t tri_count =
                        tri_bytes.size() / sizeof(DebugTriangle);
                    const std::size_t node_count =
                        bvh_bytes.size() / sizeof(DebugBvhNode);
                    logger_->info(
                        "CPU debug raytrace counts: tris={} nodes={}",
                        tri_count,
                        node_count);
                    if (tri_count > 0 && node_count > 0)
                    {
                        std::vector<DebugTriangle> tris(tri_count);
                        std::memcpy(
                            tris.data(), tri_bytes.data(), tri_bytes.size());
                        std::vector<DebugBvhNode> nodes(node_count);
                        std::memcpy(
                            nodes.data(), bvh_bytes.data(), bvh_bytes.size());
                        std::size_t bad_uv = 0;
                        std::size_t bad_pos = 0;
                        for (const auto& tri : tris)
                        {
                            const auto check_vec3 = [](const glm::vec3& v) {
                                return std::isfinite(v.x) &&
                                    std::isfinite(v.y) &&
                                    std::isfinite(v.z);
                            };
                            const auto check_vec2 = [](const glm::vec2& v) {
                                return std::isfinite(v.x) &&
                                    std::isfinite(v.y);
                            };
                            if (!check_vec3(tri.v0.position) ||
                                !check_vec3(tri.v1.position) ||
                                !check_vec3(tri.v2.position))
                            {
                                ++bad_pos;
                            }
                            if (!check_vec2(tri.v0.uv) ||
                                !check_vec2(tri.v1.uv) ||
                                !check_vec2(tri.v2.uv))
                            {
                                ++bad_uv;
                            }
                        }
                        if (bad_pos > 0 || bad_uv > 0)
                        {
                            logger_->warn(
                                "CPU debug triangle buffer has invalid data: bad_pos={} bad_uv={}",
                                bad_pos,
                                bad_uv);
                        }

                        const float px =
                            static_cast<float>(swapchain_extent_.width) * 0.5f;
                        const float py =
                            static_cast<float>(swapchain_extent_.height) * 0.5f;
                        glm::vec2 uv = (glm::vec2(px, py) + glm::vec2(0.5f)) /
                                       glm::vec2(
                                           swapchain_extent_.width,
                                           swapchain_extent_.height);
                        glm::vec2 ndc = uv * 2.0f - 1.0f;
                        glm::vec4 clip_pos(ndc, -1.0f, 1.0f);
                        glm::vec4 view_pos =
                            glm::inverse(scene_state.projection) * clip_pos;
                        view_pos = glm::vec4(
                            view_pos.x, view_pos.y, -1.0f, 0.0f);
                        glm::vec3 ray_dir_world = glm::normalize(glm::vec3(
                            glm::inverse(scene_state.view) * view_pos));
                        glm::mat4 model_inv = glm::inverse(scene_state.model);
                        glm::vec3 ray_origin = glm::vec3(
                            model_inv *
                            glm::vec4(scene_state.camera_position, 1.0f));
                        glm::vec3 ray_dir = glm::normalize(
                            glm::mat3(model_inv) * ray_dir_world);
                        logger_->info(
                            "CPU debug ray origin=({:.3f},{:.3f},{:.3f}) dir=({:.3f},{:.3f},{:.3f})",
                            ray_origin.x,
                            ray_origin.y,
                            ray_origin.z,
                            ray_dir.x,
                            ray_dir.y,
                            ray_dir.z);

                        float t_hit = 0.0f;
                        int tri_index = -1;
                        const bool cpu_hit = TraverseBvhDebug(
                            nodes, tris, ray_origin, ray_dir, t_hit, tri_index);
                        if (cpu_hit)
                        {
                            logger_->info(
                                "CPU debug ray hit tri {} at t {:.4f}",
                                tri_index,
                                t_hit);
                            if (tri_index >= 0 &&
                                static_cast<std::size_t>(tri_index) <
                                    tris.size())
                            {
                                const auto& tri = tris[tri_index];
                                logger_->info(
                                    "CPU debug tri {} v0=({:.3f},{:.3f},{:.3f}) uv=({:.3f},{:.3f}) "
                                    "v1=({:.3f},{:.3f},{:.3f}) uv=({:.3f},{:.3f}) "
                                    "v2=({:.3f},{:.3f},{:.3f}) uv=({:.3f},{:.3f})",
                                    tri_index,
                                    tri.v0.position.x,
                                    tri.v0.position.y,
                                    tri.v0.position.z,
                                    tri.v0.uv.x,
                                    tri.v0.uv.y,
                                    tri.v1.position.x,
                                    tri.v1.position.y,
                                    tri.v1.position.z,
                                    tri.v1.uv.x,
                                    tri.v1.uv.y,
                                    tri.v2.position.x,
                                    tri.v2.position.y,
                                    tri.v2.position.z,
                                    tri.v2.uv.x,
                                    tri.v2.uv.y);
                            }
                        }
                        else
                        {
                            logger_->warn(
                                "CPU debug ray missed BVH (center pixel)");
                        }
                    }
                }
                catch (const std::exception& ex)
                {
                    logger_->warn(
                        "CPU debug raytrace failed: {}", ex.what());
                }
            }
        }
    }

    auto update_uniform_buffer = [&](const SceneState& state) {
        if (!buffer_resources_)
        {
            return;
        }
        auto block = MakeUniformBlock(
            state, elapsed_time_seconds_);
        if (debug_hit_mask_)
        {
            block.time_s.w = 6.0f; // >5.5 targeted tri debug
        }
        else if (absl::GetFlag(FLAGS_vk_raytrace_bruteforce_full))
        {
            block.time_s.w = 7.0f; // >6.5 brute-force every pixel
        }
        else if (absl::GetFlag(FLAGS_vk_raytrace_bruteforce))
        {
            block.time_s.w = 2.0f; // >1.5 enables brute-force traversal
        }
        else if (absl::GetFlag(FLAGS_vk_raytrace_debug_uv))
        {
            block.time_s.w = 4.0f; // >3.5 forces UV debug output
        }
        buffer_resources_->UpdateUniform(
            &block, sizeof(UniformBlock));
    };
    update_uniform_buffer(scene_state);

    auto transition_output = [&](vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout,
                                 vk::PipelineStageFlags src_stage,
                                 vk::PipelineStageFlags dst_stage,
                                 vk::AccessFlags src_access,
                                 vk::AccessFlags dst_access) {
        if (!compute_output_image_)
        {
            return;
        }
        vk::ImageMemoryBarrier barrier(
            src_access,
            dst_access,
            old_layout,
            new_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            *compute_output_image_,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            nullptr,
            nullptr,
            barrier);
    };

    if (use_compute_raytracing_ &&
        compute_pipeline_ &&
        compute_pipeline_layout_ &&
        descriptor_set_ &&
        compute_output_image_ &&
        swapchain_extent_.width > 0 &&
        swapchain_extent_.height > 0)
    {
        if (debug_log_scene_state_ && !debug_dump_done_)
        {
            logger_->info(
                "Dispatching raytracing compute: groups=({},{}), "
                "image {}x{}, output_in_read={}",
                (swapchain_extent_.width + 7) / 8,
                (swapchain_extent_.height + 7) / 8,
                swapchain_extent_.width,
                swapchain_extent_.height,
                compute_output_in_shader_read_);
        }

        if (compute_output_in_shader_read_)
        {
            transition_output(
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eShaderWrite);
            compute_output_in_shader_read_ = false;
        }

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *compute_pipeline_);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            *compute_pipeline_layout_,
            0,
            descriptor_set_,
            {});
        const std::uint32_t group_x =
            (swapchain_extent_.width + 7) / 8;
        const std::uint32_t group_y =
            (swapchain_extent_.height + 7) / 8;
        command_buffer.dispatch(group_x, group_y, 1);

        transition_output(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
        compute_output_in_shader_read_ = true;

        // Always grab at least one pixel for debugging on the first frame.
        const bool want_full_dump = debug_dump_compute_output_;
        const bool want_single_pixel = !debug_dump_done_;
        if (want_full_dump || want_single_pixel)
        {
            const vk::DeviceSize expected_size = want_full_dump
                ? static_cast<vk::DeviceSize>(
                      swapchain_extent_.width *
                      swapchain_extent_.height *
                      sizeof(std::uint16_t) * 4)
                : static_cast<vk::DeviceSize>(sizeof(std::uint16_t) * 4);
            if (!debug_readback_.buffer ||
                debug_readback_.size != expected_size)
            {
                vk::UniqueDeviceMemory rb_memory;
                auto rb_buffer = gpu_memory_manager_->CreateBuffer(
                    expected_size,
                    vk::BufferUsageFlagBits::eTransferDst,
                    vk::MemoryPropertyFlagBits::eHostVisible |
                        vk::MemoryPropertyFlagBits::eHostCoherent,
                    rb_memory);
                debug_readback_.buffer = std::move(rb_buffer);
                debug_readback_.memory = std::move(rb_memory);
                debug_readback_.size = expected_size;
            }

            vk::ImageMemoryBarrier to_transfer(
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *compute_output_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::PipelineStageFlagBits::eTransfer,
                {},
                nullptr,
                nullptr,
                to_transfer);

            vk::BufferImageCopy copy_region(
                0,
                0,
                0,
                {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                {0, 0, 0},
                {want_full_dump ? swapchain_extent_.width : 1,
                 want_full_dump ? swapchain_extent_.height : 1,
                 1});
            command_buffer.copyImageToBuffer(
                *compute_output_image_,
                vk::ImageLayout::eTransferSrcOptimal,
                *debug_readback_.buffer,
                copy_region);

            vk::ImageMemoryBarrier back_to_shader(
                vk::AccessFlagBits::eTransferRead,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *compute_output_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {},
                nullptr,
                nullptr,
                back_to_shader);
        }
        else if (debug_log_scene_state_ && !debug_dump_done_)
        {
            // Read back a single pixel to sanity-check shader output without
            // dumping the whole image.
            const vk::DeviceSize expected_size = sizeof(std::uint16_t) * 4;
            if (!debug_readback_.buffer || debug_readback_.size != expected_size)
            {
                vk::UniqueDeviceMemory rb_memory;
                auto rb_buffer = gpu_memory_manager_->CreateBuffer(
                    expected_size,
                    vk::BufferUsageFlagBits::eTransferDst,
                    vk::MemoryPropertyFlagBits::eHostVisible |
                        vk::MemoryPropertyFlagBits::eHostCoherent,
                    rb_memory);
                debug_readback_.buffer = std::move(rb_buffer);
                debug_readback_.memory = std::move(rb_memory);
                debug_readback_.size = expected_size;
            }

            vk::ImageMemoryBarrier to_transfer(
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *compute_output_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::PipelineStageFlagBits::eTransfer,
                {},
                nullptr,
                nullptr,
                to_transfer);

            vk::BufferImageCopy copy_region(
                0,
                0,
                0,
                {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                {0, 0, 0},
                {1, 1, 1});
            command_buffer.copyImageToBuffer(
                *compute_output_image_,
                vk::ImageLayout::eTransferSrcOptimal,
                *debug_readback_.buffer,
                copy_region);

            vk::ImageMemoryBarrier back_to_shader(
                vk::AccessFlagBits::eTransferRead,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *compute_output_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {},
                nullptr,
                nullptr,
                back_to_shader);
        }
    }

    std::array<vk::ClearValue, 1> clear_values{};
    clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{
        0.1f,
        0.1f,
        0.1f,
        1.0f});

    vk::RenderPassBeginInfo render_pass_info(
        *render_pass_,
        *framebuffers_[image_index],
        vk::Rect2D({0, 0}, swapchain_extent_),
        static_cast<std::uint32_t>(clear_values.size()),
        clear_values.data());

    command_buffer.beginRenderPass(
        render_pass_info,
        vk::SubpassContents::eInline);

    if (graphics_pipeline_)
    {
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *graphics_pipeline_);

        vk::Viewport viewport(
            0.0f,
            0.0f,
            static_cast<float>(swapchain_extent_.width),
            static_cast<float>(swapchain_extent_.height),
            0.0f,
            1.0f);
        command_buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor({0, 0}, swapchain_extent_);
        command_buffer.setScissor(0, 1, &scissor);

        if (descriptor_set_layout_ && descriptor_set_)
        {
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipeline_layout_,
                0,
                descriptor_set_,
                {});
        }

        glm::mat4 projection = scene_state.projection;
        glm::mat4 view = scene_state.view;
        glm::mat4 model = scene_state.model;

        const bool needs_scene_matrices =
            push_constant_size_ > 0 &&
            !(use_procedural_quad_pipeline_ &&
              active_program_info_ &&
              active_program_info_->uses_time_uniform);

        if (needs_scene_matrices && level_)
        {
            try
            {
                Camera camera_for_frame(level_->GetDefaultCamera());
                auto camera_holder_id = level_->GetDefaultCameraId();
                if (camera_holder_id != NullId)
                {
                    auto& node =
                        level_->GetSceneNodeFromId(camera_holder_id);
                    auto matrix_node = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds_));
                    auto inverse_model = glm::inverse(matrix_node);
                    camera_for_frame.SetFront(
                        level_->GetDefaultCamera().GetFront() *
                        glm::mat3(inverse_model));
                    camera_for_frame.SetPosition(
                        glm::vec3(
                            glm::vec4(
                                level_->GetDefaultCamera().GetPosition(), 1.0f) *
                            inverse_model));
                }

                if (swapchain_extent_.height != 0)
                {
                    camera_for_frame.SetAspectRatio(
                        static_cast<float>(swapchain_extent_.width) /
                        static_cast<float>(swapchain_extent_.height));
                }
                projection = camera_for_frame.ComputeProjection();
                projection[1][1] *= -1.0f;
                view = camera_for_frame.ComputeView();
                glm::mat4 rotation = glm::mat4(1.0f);
                view = rotation * view;

                const auto mesh_pairs =
                    level_->GetStaticMeshMaterialIds();
                if (!mesh_pairs.empty())
                {
                    auto node_id = mesh_pairs.front().first;
                    auto& node = level_->GetSceneNodeFromId(node_id);
                    model = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds_));
                }
            }
            catch (const std::exception& ex)
            {
                logger_->warn(
                    "Failed to compute scene matrices: {}", ex.what());
            }
        }

        if (push_constant_size_ > 0)
        {
            if (use_procedural_quad_pipeline_ && active_program_info_ &&
                active_program_info_->uses_time_uniform)
            {
                float time = elapsed_time_seconds_;
                command_buffer.pushConstants(
                    *pipeline_layout_,
                    push_constant_stages_,
                    0,
                    push_constant_size_,
                    &time);
            }
            else
            {
                struct alignas(16) PushConstants
                {
                    glm::mat4 projection;
                    glm::mat4 view;
                    glm::mat4 model;
                    float time_s;
                } push_constants{projection, view, model, elapsed_time_seconds_};
                command_buffer.pushConstants(
                    *pipeline_layout_,
                    push_constant_stages_,
                    0,
                    push_constant_size_,
                    &push_constants);
            }
        }

        if (use_procedural_quad_pipeline_)
        {
            command_buffer.draw(6, 1, 0, 0);
        }
        else if (mesh_resources_ && !mesh_resources_->Empty())
        {
            const auto& mesh = mesh_resources_->GetMeshes().front();
            const vk::DeviceSize offsets[] = {0};
            command_buffer.bindVertexBuffers(0, *mesh.vertex_buffer, offsets);
            if (mesh.index_buffer)
            {
                command_buffer.bindIndexBuffer(
                    *mesh.index_buffer, 0, vk::IndexType::eUint32);
                command_buffer.drawIndexed(mesh.index_count, 1, 0, 0, 0);
            }
            else
            {
                command_buffer.draw(mesh.index_count, 1, 0, 0);
            }
        }
    }

    command_buffer.endRenderPass();
    command_buffer.end();
}

void Device::CreateGraphicsPipeline()
{
    if (!vk_unique_device_ || !render_pass_)
    {
        return;
    }

    DestroyGraphicsPipeline();

    if (!texture_resources_ || texture_resources_->Empty())
    {
        return;
    }

    static constexpr const char* kFallbackVertexShader = R"glsl(
        #version 450
        layout(location = 0) in vec3 in_pos;
        layout(location = 1) in vec2 in_uv;
        layout(location = 0) out vec2 v_uv;
        layout(push_constant) uniform PushConstants {
            mat4 projection;
            mat4 view;
            mat4 model;
        } pc;
        void main() {
            vec4 world = pc.model * vec4(in_pos, 1.0);
            gl_Position = pc.projection * pc.view * world;
            v_uv = in_uv;
        }
    )glsl";

    static constexpr const char* kFallbackFragmentShader = R"glsl(
        #version 450
        layout(location = 0) in vec2 v_uv;
        layout(location = 0) out vec4 out_color;
        layout(binding = 0) uniform sampler2D u_texture;
        void main() {
            out_color = texture(u_texture, v_uv);
        }
    )glsl";

    static constexpr const char* kFallbackRaytraceFragment = R"glsl(
        #version 450
        layout(location = 0) in vec2 v_uv;
        layout(location = 0) out vec4 out_color;
        void main() {
            out_color = vec4(v_uv, 0.0, 1.0);
        }
    )glsl";

    std::vector<std::uint32_t> vert_code;
    std::vector<std::uint32_t> frag_code;
    bool using_custom_program = false;
    const bool wants_raytrace_fallback =
        !use_compute_raytracing_ &&
        active_program_info_ &&
        active_program_info_->program_name == "RayTraceProgram";

    if (active_program_info_)
    {
        try
        {
            vert_code = CompileShader(
                active_program_info_->vertex_shader, shaderc_vertex_shader);
            frag_code = CompileShader(
                active_program_info_->fragment_shader, shaderc_fragment_shader);
            using_custom_program = true;
            if (wants_raytrace_fallback)
            {
                frag_code = CompileShaderSource(
                    kFallbackRaytraceFragment,
                    shaderc_fragment_shader,
                    "fallback_raytrace.frag");
                logger_->info(
                    "Compute raytracing disabled; using fallback fragment shader.");
            }
        }
        catch (const std::exception& ex)
        {
            logger_->warn(
                "Failed to compile Vulkan shader pair ({} / {}): {}",
                active_program_info_->vertex_shader.string(),
                active_program_info_->fragment_shader.string(),
                ex.what());
            vert_code.clear();
            frag_code.clear();
            using_custom_program = false;
        }
    }

    if (!using_custom_program)
    {
        vert_code = CompileShaderSource(
            kFallbackVertexShader,
            shaderc_vertex_shader,
            "frame_vulkan_default_vertex");
        frag_code = CompileShaderSource(
            kFallbackFragmentShader,
            shaderc_fragment_shader,
            "frame_vulkan_default_fragment");
    }

    use_procedural_quad_pipeline_ =
        using_custom_program &&
        active_program_info_ &&
        active_program_info_->scene_type == frame::proto::SceneType::QUAD;

    struct alignas(16) PushConstants
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        float time_s;
    };

    if (use_procedural_quad_pipeline_)
    {
        if (active_program_info_ && active_program_info_->uses_time_uniform)
        {
            push_constant_size_ = sizeof(float);
            push_constant_stages_ = vk::ShaderStageFlagBits::eFragment;
        }
        else
        {
            push_constant_size_ = 0;
            push_constant_stages_ = {};
        }
    }
    else
    {
        push_constant_size_ = static_cast<std::uint32_t>(sizeof(PushConstants));
        push_constant_stages_ = vk::ShaderStageFlagBits::eVertex;
    }

    auto vert_module = CreateShaderModule(vert_code);
    auto frag_module = CreateShaderModule(frag_code);

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, *vert_module, "main"},
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, *frag_module, "main"},
    };

    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

    if (!use_procedural_quad_pipeline_)
    {
        binding_descriptions.emplace_back(
            0,
            static_cast<std::uint32_t>(sizeof(MeshVertex)),
            vk::VertexInputRate::eVertex);
        attribute_descriptions.emplace_back(
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, position)));
        attribute_descriptions.emplace_back(
            1,
            0,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, uv)));
    }

    vk::PipelineVertexInputStateCreateInfo vertex_input_info(
        vk::PipelineVertexInputStateCreateFlags{},
        static_cast<std::uint32_t>(binding_descriptions.size()),
        binding_descriptions.data(),
        static_cast<std::uint32_t>(attribute_descriptions.size()),
        attribute_descriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo input_assembly(
        vk::PipelineInputAssemblyStateCreateFlags{},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE);

    vk::PipelineViewportStateCreateInfo viewport_state(
        vk::PipelineViewportStateCreateFlags{},
        1,
        nullptr,
        1,
        nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags{},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags{},
        vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo color_blending(
        vk::PipelineColorBlendStateCreateFlags{},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1,
        &color_blend_attachment);

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state(
        vk::PipelineDynamicStateCreateFlags{},
        static_cast<std::uint32_t>(dynamic_states.size()),
        dynamic_states.data());

    std::vector<vk::DescriptorSetLayout> set_layouts;
    if (descriptor_set_layout_)
    {
        set_layouts.push_back(*descriptor_set_layout_);
    }

    std::vector<vk::PushConstantRange> push_constant_ranges;
    if (push_constant_size_ > 0)
    {
        push_constant_ranges.emplace_back(
            push_constant_stages_, 0, push_constant_size_);
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info(
        vk::PipelineLayoutCreateFlags{},
        static_cast<std::uint32_t>(set_layouts.size()),
        set_layouts.data(),
        static_cast<std::uint32_t>(push_constant_ranges.size()),
        push_constant_ranges.empty() ? nullptr : push_constant_ranges.data());
    pipeline_layout_ = vk_unique_device_->createPipelineLayoutUnique(pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        static_cast<std::uint32_t>(std::size(shader_stages)),
        shader_stages,
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterizer,
        &multisampling,
        nullptr,
        &color_blending,
        &dynamic_state,
        *pipeline_layout_,
        *render_pass_);

    auto pipeline_result =
        vk_unique_device_->createGraphicsPipelineUnique(nullptr, pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create Vulkan graphics pipeline.");
    }
    graphics_pipeline_ = std::move(pipeline_result.value);
}

void Device::DestroyGraphicsPipeline()
{
    graphics_pipeline_.reset();
    pipeline_layout_.reset();
}

void Device::CreateComputePipeline()
{
    if (!use_compute_raytracing_ || !vk_unique_device_)
    {
        return;
    }

    DestroyComputePipeline();

    if (!descriptor_set_layout_)
    {
        logger_->warn(
            "CreateComputePipeline skipped: descriptor set layout missing.");
        return;
    }

    if (!active_program_info_ ||
        active_program_info_->compute_shader.empty())
    {
        logger_->warn(
            "CreateComputePipeline skipped: no active compute shader.");
        return;
    }

    std::vector<std::uint32_t> compute_code;
    try
    {
        compute_code = CompileShader(
            active_program_info_->compute_shader, shaderc_compute_shader);
    }
    catch (const std::exception& ex)
    {
        logger_->warn(
            "Failed to compile compute shader {}: {}",
            active_program_info_->compute_shader.string(),
            ex.what());
        return;
    }

    auto compute_module = CreateShaderModule(compute_code);

    vk::PipelineShaderStageCreateInfo stage_info(
        vk::PipelineShaderStageCreateFlags{},
        vk::ShaderStageFlagBits::eCompute,
        *compute_module,
        "main");

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{},
        1,
        layouts);
    compute_pipeline_layout_ =
        vk_unique_device_->createPipelineLayoutUnique(layout_info);

    vk::ComputePipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        stage_info,
        *compute_pipeline_layout_);

    auto pipeline_result =
        vk_unique_device_->createComputePipelineUnique(nullptr, pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create Vulkan compute pipeline.");
    }
    compute_pipeline_ = std::move(pipeline_result.value);
    logger_->info("Created raytracing compute pipeline.");
}

void Device::DestroyComputePipeline()
{
    compute_pipeline_.reset();
    compute_pipeline_layout_.reset();
    compute_output_in_shader_read_ = false;
}

std::vector<std::uint32_t> Device::CompileShader(
    const std::filesystem::path& path,
    shaderc_shader_kind kind) const
{
    auto resolve_spv_path = [](const std::filesystem::path& source_path) {
        const auto parent = source_path.parent_path();
        if (!parent.empty() && parent.filename() == "vulkan")
        {
            const auto asset_root = parent.parent_path().parent_path();
            return asset_root / "cache" / "shader" / "vulkan" /
                (source_path.filename().string() + ".spv");
        }
        return std::filesystem::path(source_path.string() + ".spv");
    };

    const std::filesystem::path spv_path = resolve_spv_path(path);
    const std::filesystem::path legacy_spv_path = path.string() + ".spv";
    std::error_code source_error;
    const bool source_exists =
        std::filesystem::exists(path, source_error) && !source_error;
    std::optional<std::filesystem::file_time_type> source_time;
    if (source_exists)
    {
        auto time = std::filesystem::last_write_time(path, source_error);
        if (!source_error)
        {
            source_time = time;
        }
    }

    auto try_load_spv = [&](const std::filesystem::path& candidate)
        -> std::optional<std::vector<std::uint32_t>> {
        std::error_code cache_error;
        if (!std::filesystem::exists(candidate, cache_error) || cache_error)
        {
            return std::nullopt;
        }
        if (source_time)
        {
            const auto cache_time =
                std::filesystem::last_write_time(candidate, cache_error);
            if (!cache_error && cache_time < *source_time)
            {
                return std::nullopt;
            }
        }
        std::ifstream cache_file(candidate, std::ios::binary);
        if (!cache_file)
        {
            return std::nullopt;
        }
        cache_file.seekg(0, std::ios::end);
        const std::streamsize cache_size = cache_file.tellg();
        cache_file.seekg(0, std::ios::beg);
        if (cache_size > 0 && (cache_size % 4) == 0)
        {
            std::vector<std::uint32_t> cached(
                static_cast<std::size_t>(cache_size / 4));
            if (cache_file.read(
                    reinterpret_cast<char*>(cached.data()),
                    cache_size))
            {
                return cached;
            }
        }
        return std::nullopt;
    };

    if (auto cached = try_load_spv(spv_path))
    {
        return *cached;
    }
    if (spv_path != legacy_spv_path)
    {
        if (auto cached = try_load_spv(legacy_spv_path))
        {
            return *cached;
        }
    }

    if (!source_exists)
    {
        throw std::runtime_error(std::format(
            "Unable to open shader file {} (no SPV cache found)",
            path.string()));
    }

    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error(std::format(
            "Unable to open shader file {}", path.string()));
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    const std::string shader_source = stream.str();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = compiler.CompileGlslToSpv(
        shader_source,
        kind,
        path.string().c_str(),
        "main",
        options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }

    std::vector<std::uint32_t> compiled{result.cbegin(), result.cend()};
    std::error_code write_error;
    std::filesystem::create_directories(
        spv_path.parent_path(), write_error);
    std::ofstream cache_out(spv_path, std::ios::binary | std::ios::trunc);
    if (cache_out)
    {
        cache_out.write(
            reinterpret_cast<const char*>(compiled.data()),
            static_cast<std::streamsize>(
                compiled.size() * sizeof(std::uint32_t)));
    }
    return compiled;
}

std::vector<std::uint32_t> Device::CompileShaderSource(
    const std::string& source,
    shaderc_shader_kind kind,
    const std::string& identifier) const
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(
        source,
        kind,
        identifier.c_str(),
        "main",
        options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
}

vk::UniqueShaderModule Device::CreateShaderModule(
    const std::vector<std::uint32_t>& code) const
{
    vk::ShaderModuleCreateInfo create_info(
        vk::ShaderModuleCreateFlags{},
        code.size() * sizeof(std::uint32_t),
        code.data());
    return vk_unique_device_->createShaderModuleUnique(create_info);
}

void Device::CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    if (command_queue_)
    {
        command_queue_->CopyBuffer(src, dst, size);
    }
}

void Device::TransitionImageLayout(
    vk::Image image,
    vk::Format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    std::uint32_t layer_count)
{
    if (command_queue_)
    {
        command_queue_->TransitionImageLayout(
            image, {}, old_layout, new_layout, layer_count);
    }
}

void Device::CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t layer_count,
    std::size_t layer_stride)
{
    if (command_queue_)
    {
        command_queue_->CopyBufferToImage(
            buffer, image, width, height, layer_count, layer_stride);
    }
}

void Device::DestroyTextureResources()
{
    if (texture_resources_)
    {
        texture_resources_->Destroy();
    }
}

void Device::DestroyDescriptorResources()
{
    descriptor_set_ = VK_NULL_HANDLE;
    descriptor_pool_.reset();
    descriptor_set_layout_.reset();
    descriptor_texture_ids_.clear();
    if (buffer_resources_)
    {
        buffer_resources_->Clear();
    }
    DestroyComputeOutputImage();
    debug_readback_ = {};
}

void Device::CreateComputeOutputImage()
{
    if (!use_compute_raytracing_ || !vk_unique_device_ || !swapchain_)
    {
        return;
    }
    if (swapchain_extent_.width == 0 || swapchain_extent_.height == 0)
    {
        return;
    }

    const auto format_props =
        vk_physical_device_.getFormatProperties(compute_output_format_);
    if (!(format_props.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eStorageImage))
    {
        throw std::runtime_error(
            "Compute output format lacks storage image support on this device.");
    }
    if (compute_output_format_ == vk::Format::eR16G16B16A16Sfloat &&
        !vk_physical_device_.getFeatures().shaderStorageImageExtendedFormats)
    {
        throw std::runtime_error(
            "shaderStorageImageExtendedFormats is required for rgba16f compute output.");
    }

    vk::Extent3D extent{
        swapchain_extent_.width,
        swapchain_extent_.height,
        1};

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        compute_output_format_,
        extent,
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    compute_output_image_ = vk_unique_device_->createImageUnique(image_info);
    auto requirements =
        vk_unique_device_->getImageMemoryRequirements(*compute_output_image_);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    compute_output_memory_ =
        vk_unique_device_->allocateMemoryUnique(allocate_info);
    vk_unique_device_->bindImageMemory(
        *compute_output_image_, *compute_output_memory_, 0);

    TransitionImageLayout(
        *compute_output_image_,
        compute_output_format_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *compute_output_image_,
        vk::ImageViewType::e2D,
        compute_output_format_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    compute_output_view_ =
        vk_unique_device_->createImageViewUnique(view_info);

    if (!compute_output_sampler_)
    {
        vk::SamplerCreateInfo sampler_info(
            vk::SamplerCreateFlags{},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            0.0f,
            VK_FALSE,
            1.0f,
            VK_FALSE,
            vk::CompareOp::eAlways,
            0.0f,
            0.0f,
            vk::BorderColor::eIntOpaqueBlack,
            VK_FALSE);
        compute_output_sampler_ =
            vk_unique_device_->createSamplerUnique(sampler_info);
    }
    compute_output_in_shader_read_ = false;
}

void Device::DestroyComputeOutputImage()
{
    compute_output_sampler_.reset();
    compute_output_view_.reset();
    compute_output_image_.reset();
    compute_output_memory_.reset();
    compute_output_in_shader_read_ = false;
}


void Device::CreateTextureResources(
    const frame::json::LevelData& level_data)
{
    if (!level_ || !texture_resources_)
    {
        return;
    }

    texture_resources_->Build(*level_, level_data);
}

void Device::CreateDescriptorResources()
{
    descriptor_texture_ids_.clear();

    if (!active_program_info_ ||
        !texture_resources_ ||
        texture_resources_->Empty() ||
        !buffer_resources_)
    {
        return;
    }

    std::vector<vk::DescriptorImageInfo> image_infos;
    if (!texture_resources_->CollectDescriptorInfos(
            active_program_info_->input_texture_ids,
            image_infos,
            descriptor_texture_ids_))
    {
        return;
    }

    buffer_resources_->Clear();
    if (level_ && active_program_info_->material_id != NullId)
    {
        buffer_resources_->BuildStorageBuffers(
            *level_, active_program_info_->input_buffer_ids);
        buffer_resources_->LogCpuBufferSamples(*level_, debug_dump_done_);
    }

    const auto& storage_buffers = buffer_resources_->GetStorageBuffers();

    const vk::DeviceSize uniform_size =
        static_cast<vk::DeviceSize>(sizeof(UniformBlock));
    buffer_resources_->BuildUniformBuffer(uniform_size);
    const BufferResource* uniform = buffer_resources_->GetUniformBuffer();
    if (!uniform)
    {
        logger_->warn("Failed to allocate Vulkan uniform buffer.");
        return;
    }

    if (use_compute_raytracing_)
    {
        DestroyComputeOutputImage();
        if (swapchain_extent_.width == 0 || swapchain_extent_.height == 0)
        {
            return;
        }
        CreateComputeOutputImage();
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(image_infos.size() + storage_buffers.size() + 4);
    const bool has_compute_output =
        use_compute_raytracing_ && compute_output_view_;

    // Binding indices for the raytracing pipeline must match the GLSL layout.
    const std::uint32_t kBindingOutputImage = 0;
    const std::uint32_t kBindingOutputSample = 1;
    const std::uint32_t kBindingTextureBase = use_compute_raytracing_ ? 2 : 0;
    const std::uint32_t kBindingTriangle = 8;
    const std::uint32_t kBindingBvh = 9;
    const std::uint32_t kBindingUniform = use_compute_raytracing_
        ? 10
        : static_cast<std::uint32_t>(
              kBindingTextureBase +
              image_infos.size() +
              storage_buffers.size());

    std::vector<std::uint32_t> texture_bindings;
    std::vector<std::uint32_t> storage_bindings;

    if (has_compute_output)
    {
        bindings.emplace_back(
            kBindingOutputImage,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute);
        bindings.emplace_back(
            kBindingOutputSample,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment |
                vk::ShaderStageFlagBits::eCompute);
    }

    vk::ShaderStageFlags texture_stages = vk::ShaderStageFlagBits::eFragment;
    if (use_compute_raytracing_)
    {
        texture_stages |= vk::ShaderStageFlagBits::eCompute;
    }
    for (std::uint32_t i = 0; i < image_infos.size(); ++i)
    {
        const std::uint32_t binding = kBindingTextureBase + i;
        texture_bindings.push_back(binding);
        bindings.emplace_back(
            binding,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            texture_stages);
    }

    vk::ShaderStageFlags buffer_stage = use_compute_raytracing_
        ? vk::ShaderStageFlagBits::eCompute
        : vk::ShaderStageFlagBits::eFragment;
    for (std::uint32_t i = 0; i < storage_buffers.size(); ++i)
    {
        std::uint32_t binding = kBindingTextureBase +
            static_cast<std::uint32_t>(image_infos.size()) + i;
        // Force fixed bindings for raytracing SSBOs: map by inner name or suffix.
        if (use_compute_raytracing_)
        {
            const std::string* inner_name = nullptr;
            if (active_program_info_ &&
                i < active_program_info_->input_buffer_inner_names.size())
            {
                inner_name = &active_program_info_->input_buffer_inner_names[i];
            }
            if (inner_name && *inner_name == "TriangleBuffer")
            {
                binding = kBindingTriangle;
            }
            else if (inner_name && *inner_name == "TriangleBufferGlass")
            {
                binding = kBindingTriangle;
            }
            else if (inner_name && *inner_name == "TriangleBufferGround")
            {
                binding = kBindingBvh;
            }
            else if (inner_name && *inner_name == "BvhBuffer")
            {
                binding = kBindingBvh;
            }
            else
            {
                const auto& name = storage_buffers[i].name;
                if (name.find(".triangle") != std::string::npos)
                {
                    binding = kBindingTriangle;
                }
                else if (name.find(".bvh") != std::string::npos)
                {
                    binding = kBindingBvh;
                }
            }
        }
        storage_bindings.push_back(binding);
        bindings.emplace_back(
            binding,
            vk::DescriptorType::eStorageBuffer,
            1,
            buffer_stage);
    }

    vk::ShaderStageFlags uniform_stages =
        vk::ShaderStageFlagBits::eFragment |
        vk::ShaderStageFlagBits::eVertex;
    if (use_compute_raytracing_)
    {
        uniform_stages |= vk::ShaderStageFlagBits::eCompute;
    }
    bindings.emplace_back(
        kBindingUniform,
        vk::DescriptorType::eUniformBuffer,
        1,
        uniform_stages);

    vk::DescriptorSetLayoutCreateInfo layout_info(
        vk::DescriptorSetLayoutCreateFlags{},
        static_cast<std::uint32_t>(bindings.size()),
        bindings.data());
    descriptor_set_layout_ =
        vk_unique_device_->createDescriptorSetLayoutUnique(layout_info);

    std::vector<vk::DescriptorPoolSize> pool_sizes;
    if (has_compute_output)
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageImage, 1);
        pool_sizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler, 1);
    }
    if (!image_infos.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<std::uint32_t>(image_infos.size()));
    }
    if (!storage_buffers.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageBuffer,
            static_cast<std::uint32_t>(storage_buffers.size()));
    }
    pool_sizes.emplace_back(
        vk::DescriptorType::eUniformBuffer, 1);

    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlags{},
        1,
        static_cast<std::uint32_t>(pool_sizes.size()),
        pool_sizes.data());
    descriptor_pool_ = vk_unique_device_->createDescriptorPoolUnique(pool_info);

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::DescriptorSetAllocateInfo alloc_info(
        *descriptor_pool_,
        1,
        layouts);
    descriptor_set_ =
        vk_unique_device_->allocateDescriptorSets(alloc_info).front();

    std::vector<vk::WriteDescriptorSet> descriptor_writes;
    descriptor_writes.reserve(bindings.size());
    if (use_compute_raytracing_ && storage_buffers.size() < 2)
    {
        logger_->warn(
            "Raytracing requires triangle/BVH buffers, but only {} storage buffers were bound.",
            storage_buffers.size());
    }
    if (use_compute_raytracing_ && !compute_output_view_)
    {
        logger_->warn("Raytracing compute output view missing; falling back to raster.");
    }
    if (has_compute_output)
    {
        vk::DescriptorImageInfo storage_image_info(
            nullptr,
            *compute_output_view_,
            vk::ImageLayout::eGeneral);
        descriptor_writes.emplace_back(
            descriptor_set_,
            kBindingOutputImage,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &storage_image_info);
        vk::DescriptorImageInfo output_sample_info(
            *compute_output_sampler_,
            *compute_output_view_,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        descriptor_writes.emplace_back(
            descriptor_set_,
            kBindingOutputSample,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &output_sample_info);
    }
    for (std::uint32_t i = 0; i < image_infos.size(); ++i)
    {
        descriptor_writes.emplace_back(
            descriptor_set_,
            texture_bindings[i],
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &image_infos[i]);
    }
    for (std::uint32_t i = 0; i < storage_buffers.size(); ++i)
    {
        vk::DescriptorBufferInfo info(
            *storage_buffers[i].buffer, 0, storage_buffers[i].size);
        descriptor_writes.emplace_back(
            descriptor_set_,
            storage_bindings[i],
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &info);
    }
    vk::DescriptorBufferInfo uniform_info(
        *uniform->buffer, 0, uniform->size);
    descriptor_writes.emplace_back(
        descriptor_set_,
        kBindingUniform,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &uniform_info);
    vk_unique_device_->updateDescriptorSets(
        static_cast<std::uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(),
        0,
        nullptr);

    if (use_compute_raytracing_)
    {
        LogDescriptorDebugInfo(
            descriptor_texture_ids_,
            texture_bindings,
            storage_buffers,
            storage_bindings);
        buffer_resources_->LogGpuBufferSamples();
        if (!compute_pipeline_ || !compute_pipeline_layout_)
        {
            logger_->warn("Raytracing compute pipeline missing after descriptor build.");
        }
    }
    else if (use_compute_raytracing_ && !compute_pipeline_)
    {
        logger_->warn("Compute dispatch skipped: compute pipeline not ready.");
    }
    else if (
        use_compute_raytracing_ &&
        (!compute_output_image_ || !descriptor_set_ ||
         swapchain_extent_.width == 0 || swapchain_extent_.height == 0))
    {
        logger_->warn(
            "Compute dispatch skipped: output_image={} descriptor_set={} size={}x{}",
            static_cast<bool>(compute_output_image_),
            static_cast<bool>(descriptor_set_),
            swapchain_extent_.width,
            swapchain_extent_.height);
    }
}

void Device::LogDescriptorDebugInfo(
    const std::vector<EntityId>& texture_ids,
    const std::vector<std::uint32_t>& texture_bindings,
    const std::vector<BufferResource>& buffers,
    const std::vector<std::uint32_t>& storage_bindings) const
{
    const auto* uniform = buffer_resources_
        ? buffer_resources_->GetUniformBuffer()
        : nullptr;
    const auto uniform_size = uniform ? uniform->size : 0;
    logger_->info(
        "Raytracing descriptor setup: {} textures, {} storage buffers, uniform size {} bytes",
        texture_ids.size(),
        buffers.size(),
        uniform_size);
    for (std::size_t i = 0; i < texture_ids.size() && i < texture_bindings.size(); ++i)
    {
        logger_->info(
            "  Texture binding {} -> id {}",
            static_cast<int>(texture_bindings[i]),
            texture_ids[i]);
    }
    for (std::size_t i = 0; i < buffers.size() && i < storage_bindings.size(); ++i)
    {
        logger_->info(
            "  Storage buffer binding {} -> {} ({} bytes)",
            static_cast<int>(storage_bindings[i]),
            buffers[i].name,
            buffers[i].size);
    }
}

} // namespace frame::vulkan
