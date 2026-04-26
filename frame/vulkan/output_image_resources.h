#pragma once

#include "frame/backend_internal.h"

#include <optional>

#include <glm/glm.hpp>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class Device;

class OutputImageResources
{
  public:
    explicit OutputImageResources(Device& device);

    void CreateComputeOutputImage();
    void DestroyComputeOutputImage();
    void CreateSwapchainPreviewImage();
    void DestroySwapchainPreviewImage();

    bool HasComputeOutputImage() const
    {
        return static_cast<bool>(compute_output_image_);
    }
    bool HasComputeOutputView() const
    {
        return static_cast<bool>(compute_output_view_);
    }
    vk::Image GetComputeOutputImage() const
    {
        return compute_output_image_.get();
    }
    vk::ImageView GetComputeOutputView() const
    {
        return compute_output_view_.get();
    }
    vk::Sampler GetComputeOutputSampler() const
    {
        return compute_output_sampler_.get();
    }
    bool IsComputeOutputInShaderRead() const
    {
        return compute_output_in_shader_read_;
    }
    void SetComputeOutputInShaderRead(bool in_shader_read)
    {
        compute_output_in_shader_read_ = in_shader_read;
    }
    bool HasSwapchainPreviewImage() const
    {
        return static_cast<bool>(swapchain_preview_image_);
    }
    vk::Image GetSwapchainPreviewImage() const
    {
        return swapchain_preview_image_.get();
    }
    bool IsSwapchainPreviewInShaderRead() const
    {
        return swapchain_preview_in_shader_read_;
    }
    void SetSwapchainPreviewInShaderRead(bool in_shader_read)
    {
        swapchain_preview_in_shader_read_ = in_shader_read;
    }
    std::optional<vk::DescriptorImageInfo> GetComputeOutputDescriptorInfo() const;
    std::optional<vk::DescriptorImageInfo> GetSwapchainPreviewDescriptorInfo() const;

  private:
    Device& device_;
    vk::UniqueImage compute_output_image_;
    vk::UniqueDeviceMemory compute_output_memory_;
    vk::UniqueImageView compute_output_view_;
    vk::UniqueSampler compute_output_sampler_;
    bool compute_output_in_shader_read_ = false;
    vk::UniqueImage swapchain_preview_image_;
    vk::UniqueDeviceMemory swapchain_preview_memory_;
    vk::UniqueImageView swapchain_preview_view_;
    vk::UniqueSampler swapchain_preview_sampler_;
    bool swapchain_preview_in_shader_read_ = false;
    vk::Format swapchain_preview_format_ = vk::Format::eUndefined;
    glm::uvec2 swapchain_preview_size_ = {0, 0};
};

} // namespace frame::vulkan
