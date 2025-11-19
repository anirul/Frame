#include "frame/vulkan/device.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include <fstream>
#include <stdexcept>
#include <sstream>
#include <shaderc/shaderc.hpp>

#include "frame/camera.h"
#include "frame/level.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/texture.h"
#include "frame/proto/uniform.pb.h"
#include <glm/gtc/packing.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace frame::vulkan
{

class TextureResources
{
  public:
    explicit TextureResources(Device& owner);

    void Build(LevelInterface& level, const frame::json::LevelData& level_data);
    void Destroy();
    bool CollectDescriptorInfos(
        const std::vector<EntityId>& requested_ids,
        std::vector<vk::DescriptorImageInfo>& out_infos,
        std::vector<EntityId>& out_ids) const;
    bool Empty() const
    {
        return textures_.empty();
    }

  private:
    void UploadTexture(EntityId id, frame::vulkan::Texture& texture);

    Device& owner_;
    std::unordered_map<EntityId, frame::vulkan::Texture*> textures_;
};


TextureResources::TextureResources(Device& owner)
    : owner_(owner)
{
}

void TextureResources::Build(
    LevelInterface& level,
    const frame::json::LevelData& level_data)
{
    Destroy();

    for (const auto& proto_texture : level_data.proto.textures())
    {
        auto texture_id = level.GetIdFromName(proto_texture.name());
        if (!texture_id)
        {
            continue;
        }

        auto* texture_ptr = dynamic_cast<frame::vulkan::Texture*>(
            &level.GetTextureFromId(texture_id));
        if (!texture_ptr)
        {
            owner_.logger_->warn(
                "Texture {} is not a Vulkan texture instance.",
                proto_texture.name());
            continue;
        }

        try
        {
            UploadTexture(texture_id, *texture_ptr);
            textures_[texture_id] = texture_ptr;
        }
        catch (const std::exception& ex)
        {
            owner_.logger_->warn(
                "Skipping texture {}: {}",
                proto_texture.name(),
                ex.what());
            texture_ptr->ResetGpuResources();
        }
    }
}

void TextureResources::Destroy()
{
    for (auto& [id, texture_ptr] : textures_)
    {
        if (texture_ptr)
        {
            texture_ptr->ResetGpuResources();
        }
    }
    textures_.clear();
}

bool TextureResources::CollectDescriptorInfos(
    const std::vector<EntityId>& requested_ids,
    std::vector<vk::DescriptorImageInfo>& out_infos,
    std::vector<EntityId>& out_ids) const
{
    out_infos.clear();
    out_ids.clear();

    for (auto texture_id : requested_ids)
    {
        const auto it = textures_.find(texture_id);
        if (it == textures_.end())
        {
            owner_.logger_->warn(
                "Texture id {} not available for descriptor binding.",
                texture_id);
            return false;
        }
        const auto* texture = it->second;
        if (!texture || !texture->HasGpuResources())
        {
            owner_.logger_->warn(
                "Texture id {} missing GPU resources.",
                texture_id);
            return false;
        }
        out_infos.push_back(texture->GetDescriptorInfo());
        out_ids.push_back(texture_id);
    }
    return true;
}

void TextureResources::UploadTexture(
    EntityId /*id*/,
    frame::vulkan::Texture& texture_interface)
{
    auto& mutable_texture =
        static_cast<frame::TextureInterface&>(texture_interface);
    const auto size = mutable_texture.GetSize();
    if (size.x == 0 || size.y == 0)
    {
        throw std::runtime_error("Texture has invalid size.");
    }

    const bool is_cubemap =
        texture_interface.GetViewType() == vk::ImageViewType::eCube;
    const auto& proto = texture_interface.GetData();
    const auto structure = proto.pixel_structure().value();
    const auto element_size = proto.pixel_element_size().value();

    const vk::Format image_format = [&] {
        switch (structure)
        {
        case frame::proto::PixelStructure::RGB_ALPHA:
        case frame::proto::PixelStructure::BGR_ALPHA:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::RGB:
        case frame::proto::PixelStructure::BGR:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::GREY:
        case frame::proto::PixelStructure::GREY_ALPHA:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::DEPTH:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eD32Sfloat;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eD16Unorm;
            default:
                break;
            }
            break;
        default:
            break;
        }
        throw std::runtime_error("Unsupported pixel format for Vulkan texture.");
    }();

    const bool is_depth =
        image_format == vk::Format::eD32Sfloat ||
        image_format == vk::Format::eD16Unorm;

    const std::size_t bytes_per_component =
        frame::vulkan::Texture::BytesPerComponent(element_size);
    const std::size_t component_count =
        frame::vulkan::Texture::ComponentCount(structure);

    if (bytes_per_component == 0 || component_count == 0)
    {
        throw std::runtime_error("Texture has unsupported component configuration.");
    }

    std::size_t src_stride = component_count * bytes_per_component;
    if (is_depth)
    {
        src_stride = bytes_per_component;
    }

    std::vector<std::uint8_t> src_data = mutable_texture.GetTextureByte();
    if (src_data.empty())
    {
        throw std::runtime_error("Texture contains no pixel data.");
    }

    const std::size_t face_count = is_cubemap ? 6 : 1;
    const std::size_t pixel_count =
        static_cast<std::size_t>(size.x) * size.y;
    const std::size_t pixel_count_total = pixel_count * face_count;
    const std::size_t expected_bytes = pixel_count_total * src_stride;

    if (!is_depth && src_data.size() < expected_bytes)
    {
        throw std::runtime_error("Texture pixel buffer is smaller than expected.");
    }

    std::vector<std::uint8_t> rgba_data;
    std::vector<std::uint8_t> linear_bytes;

    auto append_constant = [&](float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        switch (element_size)
        {
        case frame::proto::PixelElementSize::BYTE: {
            const std::uint8_t converted =
                static_cast<std::uint8_t>(std::round(clamped * 255.0f));
            rgba_data.push_back(converted);
            break;
        }
        case frame::proto::PixelElementSize::SHORT: {
            const std::uint16_t converted =
                static_cast<std::uint16_t>(std::round(clamped * 65535.0f));
            const auto* ptr =
                reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::HALF: {
            const std::uint16_t converted = glm::packHalf1x16(clamped);
            const auto* ptr =
                reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::FLOAT: {
            const float converted = clamped;
            const auto* ptr = reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        default:
            throw std::runtime_error("Unsupported element size for Vulkan texture.");
        }
    };

    if (is_depth)
    {
        linear_bytes.resize(pixel_count_total * bytes_per_component);
        std::memcpy(linear_bytes.data(), src_data.data(), linear_bytes.size());
    }
    else
    {
        rgba_data.reserve(pixel_count_total * 4 * bytes_per_component);
        for (std::size_t i = 0; i < pixel_count_total; ++i)
        {
            const auto* pixel = &src_data[i * src_stride];
            auto push_channel = [&](std::size_t channel) {
                const auto* src =
                    &pixel[channel * bytes_per_component];
                rgba_data.insert(
                    rgba_data.end(),
                    src,
                    src + bytes_per_component);
            };

            switch (structure)
            {
            case frame::proto::PixelStructure::GREY:
            case frame::proto::PixelStructure::GREY_ALPHA: {
                push_channel(0);
                push_channel(0);
                push_channel(0);
                if (component_count > 1)
                {
                    push_channel(1);
                }
                else
                {
                    append_constant(1.0f);
                }
                break;
            }
            case frame::proto::PixelStructure::RGB:
            case frame::proto::PixelStructure::BGR: {
                push_channel(structure == frame::proto::PixelStructure::BGR ? 2 : 0);
                push_channel(1);
                push_channel(structure == frame::proto::PixelStructure::BGR ? 0 : 2);
                append_constant(1.0f);
                break;
            }
            case frame::proto::PixelStructure::RGB_ALPHA:
            case frame::proto::PixelStructure::BGR_ALPHA: {
                push_channel(structure == frame::proto::PixelStructure::BGR_ALPHA ? 2 : 0);
                push_channel(1);
                push_channel(structure == frame::proto::PixelStructure::BGR_ALPHA ? 0 : 2);
                push_channel(3);
                break;
            }
            default:
                throw std::runtime_error("Unsupported pixel structure for Vulkan texture.");
            }
        }
    }

    const std::vector<std::uint8_t>& upload_data =
        is_depth ? linear_bytes : rgba_data;
    if (upload_data.empty())
    {
        throw std::runtime_error("Texture upload buffer is empty.");
    }

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = owner_.CreateBuffer(
        upload_data.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);
    void* mapped_data =
        owner_.vk_unique_device_->mapMemory(*staging_memory, 0, upload_data.size());
    std::memcpy(mapped_data, upload_data.data(), upload_data.size());
    owner_.vk_unique_device_->unmapMemory(*staging_memory);

    const std::uint32_t layer_count = face_count;

    vk::ImageCreateInfo image_info(
        is_cubemap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        image_format,
        vk::Extent3D(size.x, size.y, 1),
        1,
        layer_count,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst |
            (is_depth ? vk::ImageUsageFlagBits::eDepthStencilAttachment
                      : vk::ImageUsageFlagBits::eSampled));

    auto image = owner_.vk_unique_device_->createImageUnique(image_info);
    auto requirements = owner_.vk_unique_device_->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        owner_.FindMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    auto image_memory = owner_.vk_unique_device_->allocateMemoryUnique(allocate_info);
    owner_.vk_unique_device_->bindImageMemory(*image, *image_memory, 0);

    owner_.TransitionImageLayout(
        *image,
        image_format,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        layer_count);
    owner_.CopyBufferToImage(
        *staging_buffer,
        *image,
        size.x,
        size.y,
        layer_count,
        upload_data.size() / layer_count);
    owner_.TransitionImageLayout(
        *image,
        image_format,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        layer_count);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *image,
        texture_interface.GetViewType(),
        image_format,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});
    auto view = owner_.vk_unique_device_->createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntOpaqueBlack,
        VK_FALSE);
    auto sampler = owner_.vk_unique_device_->createSamplerUnique(sampler_info);

    texture_interface.SetGpuResources(
        image_format,
        texture_interface.GetViewType(),
        std::move(image),
        std::move(image_memory),
        std::move(view),
        std::move(sampler));
}


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

    const vk::DeviceCreateInfo device_create_info(
        {},
        static_cast<std::uint32_t>(queue_create_infos.size()),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<std::uint32_t>(device_extensions.size()),
        device_extensions.data());

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
    current_level_data_ = level_data;
    active_program_info_.reset();
    use_procedural_quad_pipeline_ = false;
    elapsed_time_seconds_ = 0.0f;

    if (!level_data.programs.empty())
    {
        ProgramPipelineInfo pipeline_info;
        const auto& program_info = level_data.programs.front();
        pipeline_info.program_name = program_info.name;
        const auto shader_root =
            level_data.asset_root / "shader" / "vulkan";
        pipeline_info.vertex_shader = shader_root / program_info.vertex_shader;
        pipeline_info.fragment_shader =
            shader_root / program_info.fragment_shader;
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

    auto built = BuildLevel(GetSize(), level_data);
    level_ = std::move(built.level);

    if (active_program_info_ && level_)
    {
        active_program_info_->program_id =
            level_->GetIdFromName(active_program_info_->program_name);
        active_program_info_->input_texture_ids.clear();
        if (active_program_info_->program_id != NullId)
        {
            try
            {
                auto& program = level_->GetProgramFromId(
                    active_program_info_->program_id);
                active_program_info_->input_texture_ids =
                    program.GetInputTextureIds();
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

    DestroyDescriptorResources();
    DestroyTextureResources();
    DestroyMeshResources();

    try
    {
        CreateTextureResources(level_data);
        CreateDescriptorResources();
        CreateMeshResources(level_data);
    }
    catch (const std::exception& ex)
    {
        logger_->error("Failed to prepare Vulkan GPU resources: {}", ex.what());
        DestroyDescriptorResources();
        DestroyTextureResources();
        DestroyMeshResources();
    }

    DestroySwapchainResources();
    CreateSwapchainResources();

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
        vk_unique_device_->waitIdle();
    }

    DestroyGraphicsPipeline();
    DestroySwapchainResources();
    DestroyDescriptorResources();
    DestroyTextureResources();
    DestroyMeshResources();
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
    const vk::Result wait_result = vk_unique_device_->waitForFences(
        *fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
    if (wait_result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for in-flight fence.");
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
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    const std::uint32_t image_index = acquire.value;
    vk_unique_device_->resetFences(*fence);

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

    graphics_queue_.submit(submit_info, *fence);

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
        throw std::runtime_error("Failed to present swapchain image.");
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

    CreateGraphicsPipeline();
}

void Device::DestroySwapchainResources()
{
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

    DestroySwapchainResources();
    CreateSwapchainResources();
}

void Device::RecordCommandBuffer(
    vk::CommandBuffer command_buffer,
    std::uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info;
    command_buffer.begin(begin_info);

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

        glm::mat4 projection(1.0f);
        glm::mat4 view(1.0f);
        glm::mat4 model(1.0f);

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
        else if (!meshes_.empty())
        {
            const auto& mesh = meshes_.front();
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

    std::vector<std::uint32_t> vert_code;
    std::vector<std::uint32_t> frag_code;
    bool using_custom_program = false;

    if (active_program_info_)
    {
        try
        {
            vert_code = CompileShader(
                active_program_info_->vertex_shader, shaderc_vertex_shader);
            frag_code = CompileShader(
                active_program_info_->fragment_shader, shaderc_fragment_shader);
            using_custom_program = true;
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
            0, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, position));
        attribute_descriptions.emplace_back(
            1, 0, vk::Format::eR32G32Sfloat, offsetof(MeshVertex, uv));
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

std::vector<std::uint32_t> Device::CompileShader(
    const std::filesystem::path& path,
    shaderc_shader_kind kind) const
{
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

    return {result.cbegin(), result.cend()};
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

std::uint32_t Device::FindMemoryType(
    std::uint32_t type_filter,
    vk::MemoryPropertyFlags properties) const
{
    auto mem_properties = vk_physical_device_.getMemoryProperties();
    for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1u << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("Unable to find suitable Vulkan memory type.");
}

vk::UniqueBuffer Device::CreateBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::UniqueDeviceMemory& out_memory) const
{
    vk::BufferCreateInfo buffer_info(
        vk::BufferCreateFlags{},
        size,
        usage,
        vk::SharingMode::eExclusive);
    auto buffer = vk_unique_device_->createBufferUnique(buffer_info);
    auto requirements = vk_unique_device_->getBufferMemoryRequirements(*buffer);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        FindMemoryType(requirements.memoryTypeBits, properties));
    out_memory = vk_unique_device_->allocateMemoryUnique(allocate_info);
    vk_unique_device_->bindBufferMemory(*buffer, *out_memory, 0);
    return buffer;
}

void Device::CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo alloc_info(
        *command_pool_,
        vk::CommandBufferLevel::ePrimary,
        1);
    auto command_buffers = vk_unique_device_->allocateCommandBuffers(alloc_info);
    auto command_buffer = command_buffers.front();
    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);
    vk::BufferCopy copy_region(0, 0, size);
    command_buffer.copyBuffer(src, dst, copy_region);
    command_buffer.end();
    vk::SubmitInfo submit_info(
        0, nullptr, nullptr,
        1, &command_buffer,
        0, nullptr);
    graphics_queue_.submit(submit_info);
    graphics_queue_.waitIdle();
    vk_unique_device_->freeCommandBuffers(*command_pool_, command_buffer);
}

void Device::TransitionImageLayout(
    vk::Image image,
    vk::Format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    std::uint32_t layer_count)
{
    vk::CommandBufferAllocateInfo alloc_info(
        *command_pool_,
        vk::CommandBufferLevel::ePrimary,
        1);
    auto command_buffers = vk_unique_device_->allocateCommandBuffers(alloc_info);
    auto command_buffer = command_buffers.front();
    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    vk::ImageMemoryBarrier barrier(
        {},
        {},
        old_layout,
        new_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});

    vk::PipelineStageFlags src_stage;
    vk::PipelineStageFlags dst_stage;

    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::runtime_error("Unsupported Vulkan image layout transition.");
    }

    command_buffer.pipelineBarrier(
        src_stage,
        dst_stage,
        {},
        nullptr,
        nullptr,
        barrier);
    command_buffer.end();

    vk::SubmitInfo submit_info(
        0, nullptr, nullptr,
        1, &command_buffer,
        0, nullptr);
    graphics_queue_.submit(submit_info);
    graphics_queue_.waitIdle();
    vk_unique_device_->freeCommandBuffers(*command_pool_, command_buffer);
}

void Device::CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t layer_count,
    std::size_t layer_stride)
{
    vk::CommandBufferAllocateInfo alloc_info(
        *command_pool_,
        vk::CommandBufferLevel::ePrimary,
        1);
    auto command_buffers = vk_unique_device_->allocateCommandBuffers(alloc_info);
    auto command_buffer = command_buffers.front();
    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    std::vector<vk::BufferImageCopy> copies;
    copies.reserve(layer_count);
    const std::size_t stride =
        (layer_stride == 0)
            ? static_cast<std::size_t>(width) * height
            : layer_stride;
    for (std::uint32_t layer = 0; layer < layer_count; ++layer)
    {
        copies.emplace_back(
            stride * layer,
            0,
            0,
            vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor, 0, layer, 1},
            vk::Offset3D{0, 0, 0},
            vk::Extent3D{width, height, 1});
    }

    command_buffer.copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        copies);

    command_buffer.end();

    vk::SubmitInfo submit_info(
        0, nullptr, nullptr,
        1, &command_buffer,
        0, nullptr);
    graphics_queue_.submit(submit_info);
    graphics_queue_.waitIdle();
    vk_unique_device_->freeCommandBuffers(*command_pool_, command_buffer);
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
        active_program_info_->input_texture_ids.empty() ||
        !texture_resources_ ||
        texture_resources_->Empty())
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

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(image_infos.size());
    for (std::uint32_t binding = 0; binding < image_infos.size(); ++binding)
    {
        bindings.emplace_back(
            binding,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment);
    }

    vk::DescriptorSetLayoutCreateInfo layout_info(
        vk::DescriptorSetLayoutCreateFlags{},
        static_cast<std::uint32_t>(bindings.size()),
        bindings.data());
    descriptor_set_layout_ =
        vk_unique_device_->createDescriptorSetLayoutUnique(layout_info);

    vk::DescriptorPoolSize pool_size(
        vk::DescriptorType::eCombinedImageSampler,
        static_cast<std::uint32_t>(image_infos.size()));
    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlags{},
        1,
        1,
        &pool_size);
    descriptor_pool_ = vk_unique_device_->createDescriptorPoolUnique(pool_info);

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::DescriptorSetAllocateInfo alloc_info(
        *descriptor_pool_,
        1,
        layouts);
    descriptor_set_ =
        vk_unique_device_->allocateDescriptorSets(alloc_info).front();

    std::vector<vk::WriteDescriptorSet> descriptor_writes;
    descriptor_writes.reserve(image_infos.size());
    for (std::uint32_t binding = 0; binding < image_infos.size(); ++binding)
    {
        descriptor_writes.emplace_back(
            descriptor_set_,
            binding,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &image_infos[binding]);
    }
    vk_unique_device_->updateDescriptorSets(
        static_cast<std::uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(),
        0,
        nullptr);
}

void Device::CreateMeshResources(
    const frame::json::LevelData& level_data)
{
    meshes_.clear();

    std::vector<frame::json::StaticMeshInfo> mesh_infos =
        level_data.meshes;

    if (mesh_infos.empty())
    {
        frame::json::StaticMeshInfo quad{};
        quad.positions = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f};
        quad.uvs = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f};
        quad.indices = {0, 1, 2, 2, 3, 0};
        mesh_infos.push_back(std::move(quad));
    }

    for (const auto& mesh_info : mesh_infos)
    {
        try
        {
            meshes_.push_back(CreateMeshResource(mesh_info));
        }
        catch (const std::exception& ex)
        {
            logger_->warn(
                "Skipping mesh {}: {}",
                mesh_info.name,
                ex.what());
        }
    }
}

void Device::DestroyMeshResources()
{
    meshes_.clear();
}

Device::MeshResource Device::CreateMeshResource(
    const frame::json::StaticMeshInfo& mesh_info)
{
    auto vertices = BuildMeshVertices(mesh_info);
    if (vertices.empty())
    {
        throw std::runtime_error("Static mesh has no vertices.");
    }

    MeshResource resource;

    const vk::DeviceSize vertex_size =
        static_cast<vk::DeviceSize>(vertices.size() * sizeof(MeshVertex));
    vk::UniqueDeviceMemory staging_vertex_memory;
    auto staging_vertex_buffer = CreateBuffer(
        vertex_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_vertex_memory);
    void* mapped_vertices =
        vk_unique_device_->mapMemory(*staging_vertex_memory, 0, vertex_size);
    std::memcpy(mapped_vertices, vertices.data(), vertex_size);
    vk_unique_device_->unmapMemory(*staging_vertex_memory);

    auto vertex_buffer = CreateBuffer(
        vertex_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        resource.vertex_memory);
    CopyBuffer(*staging_vertex_buffer, *vertex_buffer, vertex_size);
    resource.vertex_buffer = std::move(vertex_buffer);

    const auto& indices = mesh_info.indices;
    if (!indices.empty())
    {
        const vk::DeviceSize index_size =
            static_cast<vk::DeviceSize>(indices.size() * sizeof(std::uint32_t));
        vk::UniqueDeviceMemory staging_index_memory;
        auto staging_index_buffer = CreateBuffer(
            index_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_index_memory);
        void* mapped_indices =
            vk_unique_device_->mapMemory(*staging_index_memory, 0, index_size);
        std::memcpy(mapped_indices, indices.data(), index_size);
        vk_unique_device_->unmapMemory(*staging_index_memory);

        auto index_buffer = CreateBuffer(
            index_size,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            resource.index_memory);
        CopyBuffer(*staging_index_buffer, *index_buffer, index_size);
        resource.index_buffer = std::move(index_buffer);
        resource.index_count = static_cast<std::uint32_t>(indices.size());
    }
    else
    {
        resource.index_count =
            static_cast<std::uint32_t>(vertices.size());
    }

    return resource;
}

} // namespace frame::vulkan
