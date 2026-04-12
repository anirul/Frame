#include "frame/vulkan/output_image_resources.h"

#include <stdexcept>

#include "frame/vulkan/device.h"

#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/swapchain_resources.h"

namespace frame::vulkan
{

OutputImageResources::OutputImageResources(Device& device) : device_(device)
{
}

void OutputImageResources::CreateComputeOutputImage()
{
    if (!device_.use_compute_raytracing_ || !device_.vk_unique_device_ ||
        !device_.swapchain_resources_ ||
        !device_.swapchain_resources_->IsValid() ||
        !device_.gpu_memory_manager_)
    {
        return;
    }
    const auto extent2d = device_.swapchain_resources_->GetExtent();
    if (extent2d.width == 0 || extent2d.height == 0)
    {
        return;
    }

    const auto format_props =
        device_.vk_physical_device_.getFormatProperties(
            device_.compute_output_format_);
    if (!(format_props.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eStorageImage))
    {
        throw std::runtime_error(
            "Compute output format lacks storage image support on this "
            "device.");
    }
    if (device_.compute_output_format_ == vk::Format::eR16G16B16A16Sfloat &&
        !device_.vk_physical_device_.getFeatures().
            shaderStorageImageExtendedFormats)
    {
        throw std::runtime_error(
            "shaderStorageImageExtendedFormats is required for rgba16f "
            "compute output.");
    }

    vk::Extent3D extent{extent2d.width, extent2d.height, 1};

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        device_.compute_output_format_,
        extent,
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    compute_output_image_ =
        device_.vk_unique_device_->createImageUnique(image_info);
    auto requirements =
        device_.vk_unique_device_->getImageMemoryRequirements(
            *compute_output_image_);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        device_.gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    compute_output_memory_ =
        device_.vk_unique_device_->allocateMemoryUnique(allocate_info);
    device_.vk_unique_device_->bindImageMemory(
        *compute_output_image_,
        *compute_output_memory_,
        0);

    device_.TransitionImageLayout(
        *compute_output_image_,
        device_.compute_output_format_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *compute_output_image_,
        vk::ImageViewType::e2D,
        device_.compute_output_format_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    compute_output_view_ =
        device_.vk_unique_device_->createImageViewUnique(view_info);

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
            device_.vk_unique_device_->createSamplerUnique(sampler_info);
    }
    compute_output_in_shader_read_ = false;
}

void OutputImageResources::DestroyComputeOutputImage()
{
    compute_output_sampler_.reset();
    compute_output_view_.reset();
    compute_output_image_.reset();
    compute_output_memory_.reset();
    compute_output_in_shader_read_ = false;
}

void OutputImageResources::CreateSwapchainPreviewImage()
{
    if (!device_.vk_unique_device_ || !device_.swapchain_resources_ ||
        !device_.swapchain_resources_->IsValid() ||
        !device_.gpu_memory_manager_)
    {
        return;
    }

    const auto extent = device_.swapchain_resources_->GetExtent();
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    const vk::Format swapchain_format =
        device_.swapchain_resources_->GetImageFormat();
    const glm::uvec2 swapchain_size = {extent.width, extent.height};

    const bool has_valid_preview_resources =
        static_cast<bool>(swapchain_preview_image_) &&
        static_cast<bool>(swapchain_preview_view_) &&
        static_cast<bool>(swapchain_preview_sampler_);
    if (has_valid_preview_resources &&
        swapchain_preview_format_ == swapchain_format &&
        swapchain_preview_size_ == swapchain_size)
    {
        return;
    }

    DestroySwapchainPreviewImage();
    swapchain_preview_format_ = swapchain_format;
    swapchain_preview_size_ = swapchain_size;

    const auto format_props =
        device_.vk_physical_device_.getFormatProperties(
            swapchain_preview_format_);
    if (!(format_props.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImage))
    {
        device_.logger_->warn(
            "Swapchain format {} cannot be sampled; windowed Vulkan preview "
            "disabled.",
            vk::to_string(swapchain_preview_format_));
        return;
    }

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        swapchain_preview_format_,
        vk::Extent3D(extent.width, extent.height, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled);
    swapchain_preview_image_ =
        device_.vk_unique_device_->createImageUnique(image_info);

    auto requirements =
        device_.vk_unique_device_->getImageMemoryRequirements(
            *swapchain_preview_image_);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        device_.gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    swapchain_preview_memory_ =
        device_.vk_unique_device_->allocateMemoryUnique(allocate_info);
    device_.vk_unique_device_->bindImageMemory(
        *swapchain_preview_image_,
        *swapchain_preview_memory_,
        0);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *swapchain_preview_image_,
        vk::ImageViewType::e2D,
        swapchain_preview_format_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    swapchain_preview_view_ =
        device_.vk_unique_device_->createImageViewUnique(view_info);

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
    swapchain_preview_sampler_ =
        device_.vk_unique_device_->createSamplerUnique(sampler_info);
    swapchain_preview_in_shader_read_ = false;
}

void OutputImageResources::DestroySwapchainPreviewImage()
{
    swapchain_preview_sampler_.reset();
    swapchain_preview_view_.reset();
    swapchain_preview_image_.reset();
    swapchain_preview_memory_.reset();
    swapchain_preview_in_shader_read_ = false;
    swapchain_preview_format_ = vk::Format::eUndefined;
    swapchain_preview_size_ = {0, 0};
}

std::optional<vk::DescriptorImageInfo>
OutputImageResources::GetComputeOutputDescriptorInfo() const
{
    if (!compute_output_sampler_ || !compute_output_view_)
    {
        return std::nullopt;
    }
    return vk::DescriptorImageInfo(
        *compute_output_sampler_,
        *compute_output_view_,
        vk::ImageLayout::eShaderReadOnlyOptimal);
}

std::optional<vk::DescriptorImageInfo>
OutputImageResources::GetSwapchainPreviewDescriptorInfo() const
{
    if (!swapchain_preview_in_shader_read_ || !swapchain_preview_sampler_ ||
        !swapchain_preview_view_)
    {
        return std::nullopt;
    }
    return vk::DescriptorImageInfo(
        *swapchain_preview_sampler_,
        *swapchain_preview_view_,
        vk::ImageLayout::eShaderReadOnlyOptimal);
}

} // namespace frame::vulkan
