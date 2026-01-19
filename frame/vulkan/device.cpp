#include "frame/vulkan/device.h"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <filesystem>

#include <stdexcept>
#include "absl/flags/flag.h"

#include "frame/camera.h"
#include "frame/level.h"
#include "frame/common/application.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/command_resources.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/scoped_timer.h"
#include "frame/vulkan/shader_compiler.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/sync_resources.h"
#include "frame/vulkan/texture.h"
#include "frame/vulkan/texture_resources.h"
#include "frame/proto/uniform.pb.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace frame::vulkan
{

namespace
{

vk::ShaderStageFlags ToShaderStageFlags(
    const frame::proto::ProgramBinding& binding)
{
    vk::ShaderStageFlags flags{};
    for (auto stage : binding.stages())
    {
        switch (stage)
        {
        case frame::proto::ShaderStage::VERTEX:
            flags |= vk::ShaderStageFlagBits::eVertex;
            break;
        case frame::proto::ShaderStage::FRAGMENT:
            flags |= vk::ShaderStageFlagBits::eFragment;
            break;
        case frame::proto::ShaderStage::COMPUTE:
            flags |= vk::ShaderStageFlagBits::eCompute;
            break;
        case frame::proto::ShaderStage::INVALID_STAGE:
        default:
            break;
        }
    }
    return flags;
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

    // Prefer the program referenced by scene materials; otherwise fall back to
    // the first available program.
    {
        ScopedTimer timer(logger_, "Select program");
        auto pick_program = [&]() -> std::optional<frame::json::ProgramInfo> {
            auto find_program_for_material =
                [&](const std::string& material_name)
                -> std::optional<frame::json::ProgramInfo> {
                    if (material_name.empty())
                    {
                        return std::nullopt;
                    }
                    std::string program_name;
                    for (const auto& proto_material :
                         level_data.proto.materials())
                    {
                        if (proto_material.name() == material_name)
                        {
                            program_name = proto_material.program_name();
                            break;
                        }
                    }
                    if (program_name.empty())
                    {
                        return std::nullopt;
                    }
                    for (const auto& program : level_data.programs)
                    {
                        if (program.name == program_name)
                        {
                            return program;
                        }
                    }
                    return std::nullopt;
                };

            if (level_data.proto.has_scene_tree())
            {
                const auto& meshes =
                    level_data.proto.scene_tree().node_static_meshes();
                auto pick_from_meshes =
                    [&](auto predicate)
                    -> std::optional<frame::json::ProgramInfo> {
                        for (const auto& mesh : meshes)
                        {
                            if (!predicate(mesh))
                            {
                                continue;
                            }
                            if (auto program =
                                    find_program_for_material(
                                        mesh.material_name()))
                            {
                                return program;
                            }
                        }
                        return std::nullopt;
                    };

                auto is_scene_quad =
                    [&](const frame::proto::NodeStaticMesh& mesh) {
                        return mesh.render_time_enum() ==
                                   frame::proto::NodeStaticMesh::SCENE_RENDER_TIME &&
                            mesh.mesh_oneof_case() ==
                                frame::proto::NodeStaticMesh::kMeshEnum &&
                            mesh.mesh_enum() ==
                                frame::proto::NodeStaticMesh::QUAD;
                    };
                auto is_scene_mesh =
                    [&](const frame::proto::NodeStaticMesh& mesh) {
                        return mesh.render_time_enum() ==
                            frame::proto::NodeStaticMesh::SCENE_RENDER_TIME;
                    };
                auto is_non_skybox =
                    [&](const frame::proto::NodeStaticMesh& mesh) {
                        return mesh.render_time_enum() !=
                            frame::proto::NodeStaticMesh::SKYBOX_RENDER_TIME;
                    };

                if (auto program = pick_from_meshes(is_scene_quad))
                {
                    return program;
                }
                if (auto program = pick_from_meshes(is_scene_mesh))
                {
                    return program;
                }
                if (auto program = pick_from_meshes(is_non_skybox))
                {
                    return program;
                }
                if (auto program = pick_from_meshes(
                        [](const frame::proto::NodeStaticMesh&) {
                            return true;
                        }))
                {
                    return program;
                }
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
            pipeline_info.use_compute = !program_info.compute_shader.empty();
            if (pipeline_info.use_compute)
            {
                pipeline_info.compute_shader =
                    shader_root / program_info.compute_shader;
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
                    pipeline_info.bindings.clear();
                    pipeline_info.bindings.reserve(
                        static_cast<std::size_t>(proto_program.bindings_size()));
                    for (const auto& binding : proto_program.bindings())
                    {
                        ProgramPipelineInfo::BindingInfo binding_info;
                        binding_info.name = binding.name();
                        binding_info.binding = binding.binding();
                        binding_info.binding_type = binding.binding_type();
                        binding_info.stages = ToShaderStageFlags(binding);
                        pipeline_info.bindings.push_back(
                            std::move(binding_info));
                    }
                    break;
                }
            }
            active_program_info_ = std::move(pipeline_info);
        }
        else
        {
            logger_->error(
                "No Vulkan program selected from scene materials; "
                "add a program reference in the JSON material.");
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
        use_compute_raytracing_ = active_program_info_->use_compute;
        active_program_info_->texture_ids_by_inner.clear();
        active_program_info_->buffer_ids_by_inner.clear();
        active_program_info_->material_id = NullId;
        if (active_program_info_->program_id != NullId)
        {
            try
            {
                for (auto material_id : level_->GetMaterials())
                {
                    auto& material = level_->GetMaterialFromId(material_id);
                    if (material.GetProgramId(level_.get()) ==
                        active_program_info_->program_id)
                    {
                        active_program_info_->material_id = material_id;
                        for (auto texture_id : material.GetTextureIds())
                        {
                            const auto inner_name =
                                material.GetInnerName(texture_id);
                            if (!inner_name.empty())
                            {
                                active_program_info_->texture_ids_by_inner[inner_name] =
                                    texture_id;
                            }
                        }
                        for (const auto& buffer_name : material.GetBufferNames())
                        {
                            const auto inner_name =
                                material.GetInnerBufferName(buffer_name);
                            if (inner_name.empty())
                            {
                                continue;
                            }
                            const auto buffer_id =
                                level_->GetIdFromName(buffer_name);
                            if (buffer_id != NullId)
                            {
                                active_program_info_->buffer_ids_by_inner[inner_name] =
                                    buffer_id;
                            }
                        }
                        break;
                    }
                }
            }
            catch (const std::exception& ex)
            {
                logger_->warn(
                    "Failed to gather material bindings for program {}: {}",
                    active_program_info_->program_name,
                    ex.what());
            }
        }
        if (active_program_info_->material_id == NullId)
        {
            logger_->warn(
                "No material found for Vulkan program {}.",
                active_program_info_->program_name);
        }
        if (active_program_info_ && active_program_info_->use_compute &&
            active_program_info_->compute_shader.empty() &&
            current_level_data_)
        {
            logger_->warn(
                "Compute program {} has no shader_compute in JSON.",
                active_program_info_->program_name);
        }
    }

    if (!command_resources_)
    {
        command_resources_ = std::make_unique<CommandResources>(
            *vk_unique_device_, graphics_queue_family_index_);
        command_resources_->Create();
        gpu_memory_manager_ = std::make_unique<GpuMemoryManager>(
            vk_physical_device_, *vk_unique_device_);
        command_queue_ = std::make_unique<CommandQueue>(
            *vk_unique_device_,
            graphics_queue_,
            *command_resources_->GetPool());
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
    if (!swapchain_resources_)
    {
        swapchain_resources_ = std::make_unique<SwapchainResources>(
            vk_physical_device_,
            *vk_unique_device_,
            vk_surface_,
            graphics_queue_family_index_,
            present_queue_family_index_,
            logger_);
    }
    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    DestroyComputePipeline();
    DestroyGraphicsPipeline();
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
        if (swapchain_resources_)
        {
            swapchain_resources_->Destroy();
        }
        if (command_resources_)
        {
            command_resources_->FreeBuffers();
        }
        {
            ScopedTimer timer(logger_, "CreateSwapchainResources");
            if (swapchain_resources_)
            {
                swapchain_resources_->Create(size_);
            }
            if (command_resources_)
            {
                command_resources_->AllocateBuffers(
                    static_cast<std::uint32_t>(kMaxFramesInFlight));
            }
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

    if (!sync_resources_)
    {
        sync_resources_ = std::make_unique<SyncResources>(
            *vk_unique_device_, kMaxFramesInFlight);
    }
    if (sync_resources_ && !sync_resources_->IsCreated())
    {
        sync_resources_->Create();
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

    DestroyComputePipeline();
    DestroyGraphicsPipeline();
    if (swapchain_resources_)
    {
        swapchain_resources_->Destroy();
    }
    DestroyDescriptorResources();
    DestroyTextureResources();
    if (mesh_resources_)
    {
        mesh_resources_->Clear();
    }
    command_queue_.reset();
    buffer_resources_.reset();
    mesh_resources_.reset();
    gpu_memory_manager_.reset();
    if (command_resources_)
    {
        command_resources_->Destroy();
        command_resources_.reset();
    }
    if (sync_resources_)
    {
        sync_resources_->Destroy();
    }
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

    if (!vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }
    if (!command_resources_ || command_resources_->GetBuffers().empty())
    {
        return;
    }
    if (!sync_resources_ || !sync_resources_->IsCreated())
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

    const vk::Fence fence = sync_resources_->GetInFlightFence(current_frame_);
    const VkFence fence_handle = static_cast<VkFence>(fence);
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

    const auto& swapchain = swapchain_resources_->GetSwapchain();
    auto acquire = vk_unique_device_->acquireNextImageKHR(
        *swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        sync_resources_->GetImageAvailable(current_frame_),
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

    vk::CommandBuffer command_buffer =
        command_resources_->GetBuffer(current_frame_);
    command_buffer.reset();
    RecordCommandBuffer(command_buffer, image_index);

    const vk::Semaphore wait_semaphores[] = {
        sync_resources_->GetImageAvailable(current_frame_)};
    const vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::Semaphore signal_semaphores[] = {
        sync_resources_->GetRenderFinished(current_frame_)};

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
        fence);
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

    vk::PresentInfoKHR present_info(
        1,
        signal_semaphores,
        1,
        &swapchain.get(),
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

void Device::RecreateSwapchain()
{
    if (size_.x == 0 || size_.y == 0)
    {
        return;
    }
    if (!swapchain_resources_ || !command_resources_)
    {
        return;
    }
    if (vk_unique_device_)
    {
        vk_unique_device_->waitIdle();
    }

    DestroyComputePipeline();
    DestroyDescriptorResources();
    DestroyGraphicsPipeline();
    swapchain_resources_->Destroy();
    command_resources_->FreeBuffers();

    swapchain_resources_->Create(size_);
    command_resources_->AllocateBuffers(
        static_cast<std::uint32_t>(kMaxFramesInFlight));

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

    const auto extent = swapchain_resources_->GetExtent();
    const auto& render_pass = swapchain_resources_->GetRenderPass();
    const auto& framebuffers = swapchain_resources_->GetFramebuffers();

    std::string preferred_scene_root;
    if (level_ && active_program_info_ &&
        active_program_info_->program_id != NullId)
    {
        preferred_scene_root =
            level_->GetProgramFromId(active_program_info_->program_id)
                .GetTemporarySceneRoot();
    }

    const SceneState scene_state =
        (level_)
            ? BuildSceneState(
                  *level_,
                  frame::Logger::GetInstance(),
                  {extent.width, extent.height},
                  elapsed_time_seconds_,
                  active_program_info_
                      ? active_program_info_->material_id
                      : NullId,
                  !use_compute_raytracing_,
                  preferred_scene_root)
            : SceneState{};

    auto update_uniform_buffer = [&](const SceneState& state) {
        if (!buffer_resources_)
        {
            return;
        }
        if (!buffer_resources_->GetUniformBuffer())
        {
            return;
        }
        auto block = MakeUniformBlock(
            state, elapsed_time_seconds_);
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
        extent.width > 0 &&
        extent.height > 0)
    {
        if (!storage_buffers_ready_ && buffer_resources_)
        {
            const auto& storage_buffers =
                buffer_resources_->GetStorageBuffers();
            std::vector<vk::BufferMemoryBarrier> buffer_barriers;
            buffer_barriers.reserve(storage_buffers.size());
            for (const auto& storage : storage_buffers)
            {
                if (!storage.buffer || storage.size == 0)
                {
                    continue;
                }
                buffer_barriers.emplace_back(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eShaderRead,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    *storage.buffer,
                    0,
                    storage.size);
            }
            if (!buffer_barriers.empty())
            {
                command_buffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eComputeShader,
                    {},
                    nullptr,
                    buffer_barriers,
                    nullptr);
            }
            storage_buffers_ready_ = true;
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
            (extent.width + 7) / 8;
        const std::uint32_t group_y =
            (extent.height + 7) / 8;
        command_buffer.dispatch(group_x, group_y, 1);

        transition_output(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
        compute_output_in_shader_read_ = true;
    }

    std::array<vk::ClearValue, 1> clear_values{};
    clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{
        0.1f,
        0.1f,
        0.1f,
        1.0f});

    vk::RenderPassBeginInfo render_pass_info(
        *render_pass,
        *framebuffers[image_index],
        vk::Rect2D({0, 0}, extent),
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
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f,
            1.0f);
        command_buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor({0, 0}, extent);
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

                if (extent.height != 0)
                {
                    camera_for_frame.SetAspectRatio(
                        static_cast<float>(extent.width) /
                        static_cast<float>(extent.height));
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
    if (!vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }

    const auto& render_pass = swapchain_resources_->GetRenderPass();

    DestroyGraphicsPipeline();

    if (!texture_resources_ || texture_resources_->Empty())
    {
        return;
    }

    std::vector<std::uint32_t> vert_code;
    std::vector<std::uint32_t> frag_code;
    if (!active_program_info_)
    {
        logger_->error(
            "No Vulkan program selected; cannot build graphics pipeline.");
        return;
    }
    if (active_program_info_->vertex_shader.empty() ||
        active_program_info_->fragment_shader.empty())
    {
        logger_->error(
            "Vulkan program {} is missing shader filenames.",
            active_program_info_->program_name);
        return;
    }

    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    try
    {
        vert_code = shader_compiler_->CompileFile(
            active_program_info_->vertex_shader, shaderc_vertex_shader);
        frag_code = shader_compiler_->CompileFile(
            active_program_info_->fragment_shader, shaderc_fragment_shader);
    }
    catch (const std::exception& ex)
    {
        logger_->warn(
            "Failed to compile Vulkan shader pair ({} / {}): {}",
            active_program_info_->vertex_shader.string(),
            active_program_info_->fragment_shader.string(),
            ex.what());
        return;
    }

    use_procedural_quad_pipeline_ =
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
        *render_pass);

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

    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    std::vector<std::uint32_t> compute_code;
    try
    {
        compute_code = shader_compiler_->CompileFile(
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
    if (buffer_resources_)
    {
        buffer_resources_->Clear();
    }
    storage_buffers_ready_ = false;
    DestroyComputeOutputImage();
}

void Device::CreateComputeOutputImage()
{
    if (!use_compute_raytracing_ || !vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }
    const auto extent2d = swapchain_resources_->GetExtent();
    if (extent2d.width == 0 || extent2d.height == 0)
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
        extent2d.width,
        extent2d.height,
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
    descriptor_set_ = VK_NULL_HANDLE;
    descriptor_pool_.reset();
    descriptor_set_layout_.reset();
    storage_buffers_ready_ = false;

    if (!active_program_info_ || !buffer_resources_ || !texture_resources_)
    {
        return;
    }

    const auto& binding_defs = active_program_info_->bindings;
    if (binding_defs.empty())
    {
        return;
    }

    std::unordered_set<std::uint32_t> used_bindings;
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    layout_bindings.reserve(binding_defs.size());

    std::vector<std::uint32_t> texture_bindings;
    std::vector<EntityId> texture_ids;
    std::vector<std::uint32_t> storage_bindings;
    std::vector<EntityId> storage_ids;
    std::vector<std::uint32_t> uniform_bindings;
    std::vector<std::uint32_t> output_image_bindings;
    std::vector<std::uint32_t> output_sampler_bindings;

    for (const auto& binding : binding_defs)
    {
        if (binding.stages == vk::ShaderStageFlags{})
        {
            logger_->error(
                "Program {} binding {} has no shader stages.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }
        if (!used_bindings.insert(binding.binding).second)
        {
            logger_->error(
                "Program {} has duplicate binding index {}.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }

        vk::DescriptorType descriptor_type{};
        switch (binding.binding_type)
        {
        case frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER:
            descriptor_type = vk::DescriptorType::eCombinedImageSampler;
            if (binding.name.empty())
            {
                logger_->error(
                    "Program {} texture binding {} has no name.",
                    active_program_info_->program_name,
                    binding.binding);
                return;
            }
            if (auto it =
                    active_program_info_->texture_ids_by_inner.find(binding.name);
                it != active_program_info_->texture_ids_by_inner.end())
            {
                texture_bindings.push_back(binding.binding);
                texture_ids.push_back(it->second);
            }
            else
            {
                logger_->error(
                    "Program {} missing texture '{}' for binding {}.",
                    active_program_info_->program_name,
                    binding.name,
                    binding.binding);
                return;
            }
            break;
        case frame::proto::ProgramBinding::OUTPUT_SAMPLER:
            descriptor_type = vk::DescriptorType::eCombinedImageSampler;
            output_sampler_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::STORAGE_IMAGE:
            descriptor_type = vk::DescriptorType::eStorageImage;
            output_image_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::STORAGE_BUFFER:
            descriptor_type = vk::DescriptorType::eStorageBuffer;
            if (binding.name.empty())
            {
                logger_->error(
                    "Program {} storage buffer binding {} has no name.",
                    active_program_info_->program_name,
                    binding.binding);
                return;
            }
            if (auto it =
                    active_program_info_->buffer_ids_by_inner.find(binding.name);
                it != active_program_info_->buffer_ids_by_inner.end())
            {
                storage_bindings.push_back(binding.binding);
                storage_ids.push_back(it->second);
            }
            else
            {
                logger_->error(
                    "Program {} missing buffer '{}' for binding {}.",
                    active_program_info_->program_name,
                    binding.name,
                    binding.binding);
                return;
            }
            break;
        case frame::proto::ProgramBinding::UNIFORM_BUFFER:
            descriptor_type = vk::DescriptorType::eUniformBuffer;
            uniform_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::BINDING_INVALID:
        default:
            logger_->error(
                "Program {} has invalid binding type at index {}.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }

        layout_bindings.emplace_back(
            binding.binding,
            descriptor_type,
            1,
            binding.stages);
    }

    if (!output_image_bindings.empty() || !output_sampler_bindings.empty())
    {
        if (!use_compute_raytracing_)
        {
            logger_->error(
                "Program {} declares compute output bindings but compute is disabled.",
                active_program_info_->program_name);
            return;
        }
        if (output_image_bindings.empty())
        {
            logger_->error(
                "Program {} is missing a storage image binding for compute output.",
                active_program_info_->program_name);
            return;
        }
    }

    if (uniform_bindings.size() > 1)
    {
        logger_->error(
            "Program {} declares {} uniform bindings; only one is supported.",
            active_program_info_->program_name,
            uniform_bindings.size());
        return;
    }

    buffer_resources_->Clear();
    if (!storage_ids.empty())
    {
        if (!level_)
        {
            logger_->error(
                "Program {} requested buffers without a level.",
                active_program_info_->program_name);
            return;
        }
        buffer_resources_->BuildStorageBuffers(*level_, storage_ids);
    }

    const auto& storage_buffers = buffer_resources_->GetStorageBuffers();
    if (storage_buffers.size() != storage_ids.size())
    {
        logger_->error(
            "Program {} storage buffer count mismatch ({} vs {}).",
            active_program_info_->program_name,
            storage_buffers.size(),
            storage_ids.size());
        return;
    }


    const BufferResource* uniform = nullptr;
    if (!uniform_bindings.empty())
    {
        buffer_resources_->BuildUniformBuffer(
            static_cast<vk::DeviceSize>(sizeof(UniformBlock)));
        uniform = buffer_resources_->GetUniformBuffer();
        if (!uniform)
        {
            logger_->error("Failed to allocate Vulkan uniform buffer.");
            return;
        }
    }

    std::vector<vk::DescriptorImageInfo> texture_infos;
    if (!texture_ids.empty())
    {
        if (texture_resources_->Empty())
        {
            logger_->error(
                "Program {} requires textures but none are loaded.",
                active_program_info_->program_name);
            return;
        }
        std::vector<EntityId> resolved_ids;
        if (!texture_resources_->CollectDescriptorInfos(
                texture_ids,
                texture_infos,
                resolved_ids))
        {
            return;
        }
        if (texture_infos.size() != texture_ids.size())
        {
            logger_->error(
                "Program {} texture descriptor count mismatch.",
                active_program_info_->program_name);
            return;
        }
    }

    const bool needs_compute_output =
        !output_image_bindings.empty() || !output_sampler_bindings.empty();
    if (needs_compute_output)
    {
        DestroyComputeOutputImage();
        if (!swapchain_resources_ || !swapchain_resources_->IsValid())
        {
            return;
        }
        const auto extent = swapchain_resources_->GetExtent();
        if (extent.width == 0 || extent.height == 0)
        {
            return;
        }
        CreateComputeOutputImage();
        if (!compute_output_view_)
        {
            logger_->error(
                "Failed to create compute output image for program {}.",
                active_program_info_->program_name);
            return;
        }
    }

    vk::DescriptorSetLayoutCreateInfo layout_info(
        vk::DescriptorSetLayoutCreateFlags{},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
    descriptor_set_layout_ =
        vk_unique_device_->createDescriptorSetLayoutUnique(layout_info);

    std::vector<vk::DescriptorPoolSize> pool_sizes;
    const std::uint32_t combined_count =
        static_cast<std::uint32_t>(texture_bindings.size() +
                                   output_sampler_bindings.size());
    if (combined_count > 0)
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler,
            combined_count);
    }
    if (!output_image_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageImage,
            static_cast<std::uint32_t>(output_image_bindings.size()));
    }
    if (!storage_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageBuffer,
            static_cast<std::uint32_t>(storage_bindings.size()));
    }
    if (!uniform_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eUniformBuffer,
            static_cast<std::uint32_t>(uniform_bindings.size()));
    }
    if (pool_sizes.empty())
    {
        return;
    }

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

    std::vector<vk::DescriptorImageInfo> output_image_infos;
    std::vector<vk::DescriptorImageInfo> output_sampler_infos;
    std::vector<vk::DescriptorBufferInfo> storage_infos;
    std::vector<vk::DescriptorBufferInfo> uniform_infos;
    std::vector<vk::WriteDescriptorSet> descriptor_writes;
    descriptor_writes.reserve(layout_bindings.size());
    output_image_infos.reserve(output_image_bindings.size());
    output_sampler_infos.reserve(output_sampler_bindings.size());
    storage_infos.reserve(storage_buffers.size());
    uniform_infos.reserve(uniform_bindings.size());

    for (auto binding : output_image_bindings)
    {
        output_image_infos.emplace_back(
            nullptr,
            *compute_output_view_,
            vk::ImageLayout::eGeneral);
        descriptor_writes.emplace_back(
            descriptor_set_,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &output_image_infos.back());
    }

    for (auto binding : output_sampler_bindings)
    {
        output_sampler_infos.emplace_back(
            *compute_output_sampler_,
            *compute_output_view_,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        descriptor_writes.emplace_back(
            descriptor_set_,
            binding,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &output_sampler_infos.back());
    }

    for (std::size_t i = 0; i < texture_infos.size(); ++i)
    {
        descriptor_writes.emplace_back(
            descriptor_set_,
            texture_bindings[i],
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos[i]);
    }

    for (std::size_t i = 0; i < storage_buffers.size(); ++i)
    {
        storage_infos.emplace_back(
            *storage_buffers[i].buffer,
            0,
            storage_buffers[i].size);
        descriptor_writes.emplace_back(
            descriptor_set_,
            storage_bindings[i],
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &storage_infos.back());
    }

    if (uniform && !uniform_bindings.empty())
    {
        uniform_infos.emplace_back(
            *uniform->buffer,
            0,
            uniform->size);
        descriptor_writes.emplace_back(
            descriptor_set_,
            uniform_bindings.front(),
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &uniform_infos.back());
    }

    vk_unique_device_->updateDescriptorSets(
        static_cast<std::uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(),
        0,
        nullptr);
}


} // namespace frame::vulkan
