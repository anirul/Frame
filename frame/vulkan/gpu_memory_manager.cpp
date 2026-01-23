#include "frame/vulkan/gpu_memory_manager.h"

#include <stdexcept>

namespace frame::vulkan
{

std::uint32_t GpuMemoryManager::FindMemoryType(
    std::uint32_t type_filter, vk::MemoryPropertyFlags properties) const
{
    auto mem_properties = physical_device_.getMemoryProperties();
    for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1u << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }
    throw std::runtime_error("Unable to find suitable Vulkan memory type.");
}

vk::UniqueBuffer GpuMemoryManager::CreateBuffer(
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
    auto buffer = device_.createBufferUnique(buffer_info);
    auto requirements = device_.getBufferMemoryRequirements(*buffer);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        FindMemoryType(requirements.memoryTypeBits, properties));
    out_memory = device_.allocateMemoryUnique(allocate_info);
    device_.bindBufferMemory(*buffer, *out_memory, 0);
    return buffer;
}

} // namespace frame::vulkan
