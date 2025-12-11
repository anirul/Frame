#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include <vulkan/vulkan.hpp>

namespace frame::vulkan
{

class GpuMemoryManager
{
  public:
    GpuMemoryManager(vk::PhysicalDevice physical_device, vk::Device device)
        : physical_device_(physical_device), device_(device)
    {
    }

    std::uint32_t FindMemoryType(
        std::uint32_t type_filter, vk::MemoryPropertyFlags properties) const;

    vk::UniqueBuffer CreateBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::UniqueDeviceMemory& out_memory) const;

  private:
    vk::PhysicalDevice physical_device_;
    vk::Device device_;
};

} // namespace frame::vulkan
