#include "frame/vulkan/swapchain_resources.h"

#include <algorithm>
#include <array>
#include <limits>

namespace frame::vulkan
{

SwapchainResources::SwapchainResources(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    vk::SurfaceKHR surface,
    std::uint32_t graphics_queue_family_index,
    std::uint32_t present_queue_family_index,
    const Logger& logger)
    : physical_device_(physical_device),
      device_(device),
      surface_(surface),
      graphics_queue_family_index_(graphics_queue_family_index),
      present_queue_family_index_(present_queue_family_index),
      logger_(logger)
{
}

void SwapchainResources::Create(glm::uvec2 size)
{
    Destroy();

    const auto capabilities =
        physical_device_.getSurfaceCapabilitiesKHR(surface_);
    const auto formats =
        physical_device_.getSurfaceFormatsKHR(surface_);
    const auto present_modes =
        physical_device_.getSurfacePresentModesKHR(surface_);

    const vk::SurfaceFormatKHR surface_format =
        SelectSurfaceFormat(formats);
    const vk::PresentModeKHR present_mode =
        SelectPresentMode(present_modes);
    const vk::Extent2D extent = SelectSwapExtent(capabilities, size);

    std::uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_info(
        {},
        surface_,
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

    swapchain_ = device_.createSwapchainKHRUnique(swapchain_info);
    images_ = device_.getSwapchainImagesKHR(*swapchain_);
    image_format_ = surface_format.format;
    extent_ = extent;

    image_views_.clear();
    image_views_.reserve(images_.size());
    for (const auto& image : images_)
    {
        vk::ImageViewCreateInfo view_info(
            {},
            image,
            vk::ImageViewType::e2D,
            image_format_,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        image_views_.push_back(
            device_.createImageViewUnique(view_info));
    }

    vk::AttachmentDescription color_attachment(
        {},
        image_format_,
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

    render_pass_ = device_.createRenderPassUnique(render_pass_info);

    framebuffers_.clear();
    framebuffers_.reserve(image_views_.size());
    for (const auto& view : image_views_)
    {
        vk::FramebufferCreateInfo framebuffer_info(
            {},
            *render_pass_,
            1,
            &view.get(),
            extent_.width,
            extent_.height,
            1);
        framebuffers_.push_back(
            device_.createFramebufferUnique(framebuffer_info));
    }

    logger_->info(
        "Created Vulkan swapchain ({}x{}, {} images).",
        extent_.width,
        extent_.height,
        static_cast<unsigned int>(images_.size()));
}

void SwapchainResources::Destroy()
{
    framebuffers_.clear();
    render_pass_.reset();
    image_views_.clear();
    images_.clear();
    swapchain_.reset();
}

void SwapchainResources::Recreate(glm::uvec2 size)
{
    Destroy();
    Create(size);
}

vk::SurfaceFormatKHR SwapchainResources::SelectSurfaceFormat(
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

vk::PresentModeKHR SwapchainResources::SelectPresentMode(
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

vk::Extent2D SwapchainResources::SelectSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities,
    glm::uvec2 size) const
{
    if (capabilities.currentExtent.width !=
        std::numeric_limits<std::uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    vk::Extent2D actual_extent{
        static_cast<std::uint32_t>(size.x),
        static_cast<std::uint32_t>(size.y)};

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

} // namespace frame::vulkan
