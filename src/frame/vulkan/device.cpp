#include "frame/vulkan/device.h"

#include <algorithm>
#include <array>
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

#include "frame/level.h"
#include "frame/vulkan/build_level.h"

namespace frame::vulkan
{

Device::Device(
    void* vk_instance,
    glm::uvec2 size,
    vk::SurfaceKHR& surface)
    : vk_instance_(static_cast<VkInstance>(vk_instance)),
      size_(size),
      vk_surface_(surface)
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
    elapsed_time_seconds_ = 0.0f;

    auto built = BuildLevel(GetSize(), level_data);
    level_ = std::move(built.level);

    if (!command_pool_)
    {
        CreateCommandPool();
    }

    DestroyDescriptorResources();
    DestroyTextureResources();

    try
    {
        CreateTextureResources(level_data);
        CreateDescriptorResources();
    }
    catch (const std::exception& ex)
    {
        logger_->error("Failed to prepare Vulkan GPU resources: {}", ex.what());
        DestroyDescriptorResources();
        DestroyTextureResources();
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
    elapsed_time_seconds_ = static_cast<float>(dt);

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

        command_buffer.draw(6, 1, 0, 0);
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

    if (textures_.empty())
    {
        return;
    }

    static constexpr const char* kDefaultVertexShader = R"glsl(
        #version 450
        layout(location = 0) out vec2 v_uv;
        vec2 positions[3] = vec2[](
            vec2(-1.0, -1.0),
            vec2(3.0, -1.0),
            vec2(-1.0, 3.0)
        );
        vec2 uvs[3] = vec2[](
            vec2(0.0, 0.0),
            vec2(2.0, 0.0),
            vec2(0.0, 2.0)
        );
        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            v_uv = uvs[gl_VertexIndex];
        }
    )glsl";

    static constexpr const char* kDefaultFragmentShader = R"glsl(
        #version 450
        layout(location = 0) in vec2 v_uv;
        layout(location = 0) out vec4 out_color;
        layout(binding = 0) uniform sampler2D u_texture;
        void main() {
            out_color = texture(u_texture, v_uv);
        }
    )glsl";

    auto vert_code = CompileShaderSource(
        kDefaultVertexShader,
        shaderc_vertex_shader,
        "frame_vulkan_default_vertex");
    auto frag_code = CompileShaderSource(
        kDefaultFragmentShader,
        shaderc_fragment_shader,
        "frame_vulkan_default_fragment");

    auto vert_module = CreateShaderModule(vert_code);
    auto frag_module = CreateShaderModule(frag_code);

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, *vert_module, "main"},
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, *frag_module, "main"},
    };

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};

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

    vk::PipelineLayoutCreateInfo pipeline_layout_info(
        vk::PipelineLayoutCreateFlags{},
        static_cast<std::uint32_t>(set_layouts.size()),
        set_layouts.data());
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
    vk::ImageLayout new_layout)
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
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

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
    std::uint32_t height)
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

    vk::BufferImageCopy copy_region(
        0,
        0,
        0,
        {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        {0, 0, 0},
        {width, height, 1});

    command_buffer.copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        copy_region);

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
    textures_.clear();
}

void Device::DestroyDescriptorResources()
{
    descriptor_set_ = VK_NULL_HANDLE;
    descriptor_pool_.reset();
    descriptor_set_layout_.reset();
}

Device::TextureResource Device::CreateTextureFromCpu(
    const frame::TextureInterface& texture_interface)
{
    auto& mutable_texture =
        const_cast<frame::TextureInterface&>(texture_interface);
    const auto size = mutable_texture.GetSize();
    if (size.x == 0 || size.y == 0)
    {
        throw std::runtime_error("Texture has invalid size.");
    }

    const auto& proto = texture_interface.GetData();
    if (proto.pixel_element_size().value() != frame::proto::PixelElementSize::BYTE)
    {
        throw std::runtime_error("Only 8-bit textures are currently supported.");
    }

    std::vector<std::uint8_t> src_data = mutable_texture.GetTextureByte();
    if (src_data.empty())
    {
        throw std::runtime_error("Texture contains no pixel data.");
    }

    std::vector<std::uint8_t> rgba_data;
    const auto structure = proto.pixel_structure().value();
    std::uint32_t component_count = 0;
    switch (structure)
    {
    case frame::proto::PixelStructure::GREY:
        component_count = 1;
        break;
    case frame::proto::PixelStructure::GREY_ALPHA:
        component_count = 2;
        break;
    case frame::proto::PixelStructure::RGB:
        component_count = 3;
        break;
    case frame::proto::PixelStructure::RGB_ALPHA:
        component_count = 4;
        break;
    case frame::proto::PixelStructure::BGR:
        component_count = 3;
        break;
    case frame::proto::PixelStructure::BGR_ALPHA:
        component_count = 4;
        break;
    default:
        throw std::runtime_error("Unsupported pixel structure for Vulkan texture.");
    }

    const std::size_t pixel_count = src_data.size() / component_count;
    rgba_data.reserve(pixel_count * 4);
    for (std::size_t i = 0; i < pixel_count; ++i)
    {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;
        switch (component_count)
        {
        case 1: {
            const std::uint8_t v = src_data[i];
            r = g = b = v;
            break;
        }
        case 2: {
            const std::uint8_t v = src_data[i * 2 + 0];
            r = g = b = v;
            a = src_data[i * 2 + 1];
            break;
        }
        case 3: {
            r = src_data[i * 3 + 0];
            g = src_data[i * 3 + 1];
            b = src_data[i * 3 + 2];
            break;
        }
        case 4: {
            r = src_data[i * 4 + 0];
            g = src_data[i * 4 + 1];
            b = src_data[i * 4 + 2];
            a = src_data[i * 4 + 3];
            break;
        }
        }
        if (structure == frame::proto::PixelStructure::BGR ||
            structure == frame::proto::PixelStructure::BGR_ALPHA)
        {
            std::swap(r, b);
        }
        rgba_data.push_back(r);
        rgba_data.push_back(g);
        rgba_data.push_back(b);
        rgba_data.push_back(a);
    }

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = CreateBuffer(
        rgba_data.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);
    void* mapped_data =
        vk_unique_device_->mapMemory(*staging_memory, 0, rgba_data.size());
    std::memcpy(mapped_data, rgba_data.data(), rgba_data.size());
    vk_unique_device_->unmapMemory(*staging_memory);

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(size.x, size.y, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

    auto image = vk_unique_device_->createImageUnique(image_info);
    auto requirements = vk_unique_device_->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        FindMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    auto image_memory = vk_unique_device_->allocateMemoryUnique(allocate_info);
    vk_unique_device_->bindImageMemory(*image, *image_memory, 0);

    TransitionImageLayout(
        *image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);
    CopyBufferToImage(
        *staging_buffer,
        *image,
        size.x,
        size.y);
    TransitionImageLayout(
        *image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *image,
        vk::ImageViewType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    auto view = vk_unique_device_->createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntOpaqueBlack,
        VK_FALSE);
    auto sampler = vk_unique_device_->createSamplerUnique(sampler_info);

    TextureResource resource;
    resource.image = std::move(image);
    resource.memory = std::move(image_memory);
    resource.view = std::move(view);
    resource.sampler = std::move(sampler);
    resource.size = size;
    return resource;
}

void Device::CreateTextureResources(
    const frame::json::LevelData& level_data)
{
    if (!level_)
    {
        return;
    }

    for (const auto& proto_texture : level_data.proto.textures())
    {
        auto texture_id = level_->GetIdFromName(proto_texture.name());
        if (!texture_id)
        {
            continue;
        }
        auto& texture_interface = level_->GetTextureFromId(texture_id);
        try
        {
            textures_.push_back(CreateTextureFromCpu(texture_interface));
        }
        catch (const std::exception& ex)
        {
            logger_->warn(
                "Skipping texture {}: {}",
                proto_texture.name(),
                ex.what());
        }
    }
}

void Device::CreateDescriptorResources()
{
    if (textures_.empty())
    {
        return;
    }

    vk::DescriptorSetLayoutBinding binding(
        0,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment);
    vk::DescriptorSetLayoutCreateInfo layout_info(
        vk::DescriptorSetLayoutCreateFlags{},
        1,
        &binding);
    descriptor_set_layout_ =
        vk_unique_device_->createDescriptorSetLayoutUnique(layout_info);

    vk::DescriptorPoolSize pool_size(
        vk::DescriptorType::eCombinedImageSampler,
        1);
    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlags{},
        1,
        1,
        &pool_size);
    descriptor_pool_ = vk_unique_device_->createDescriptorPoolUnique(pool_info);

    vk::DescriptorSetAllocateInfo alloc_info(
        *descriptor_pool_,
        1,
        &descriptor_set_layout_.get());
    descriptor_set_ =
        vk_unique_device_->allocateDescriptorSets(alloc_info).front();

    vk::DescriptorImageInfo image_info(
        *textures_.front().sampler,
        *textures_.front().view,
        vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet descriptor_write(
        descriptor_set_,
        0,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &image_info);
    vk_unique_device_->updateDescriptorSets(descriptor_write, nullptr);
}

} // namespace frame::vulkan
