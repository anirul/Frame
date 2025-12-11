#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include <vulkan/vulkan.hpp>

namespace frame::vulkan
{

class CommandQueue
{
  public:
    CommandQueue(vk::Device device, vk::Queue queue, vk::CommandPool pool)
        : device_(device), queue_(queue), pool_(pool)
    {
    }

    void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) const;
    void CopyBufferToImage(
        vk::Buffer buffer,
        vk::Image image,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t layer_count = 1,
        std::size_t layer_stride = 0) const;
    void TransitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        std::uint32_t layer_count = 1) const;

  private:
    vk::CommandBuffer BeginOneTime() const;
    void EndOneTime(vk::CommandBuffer command_buffer) const;

  private:
    vk::Device device_;
    vk::Queue queue_;
    vk::CommandPool pool_;
};

} // namespace frame::vulkan
