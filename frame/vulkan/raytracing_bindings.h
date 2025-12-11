#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace frame::vulkan
{

struct RayTracingBindingState
{
    bool has_compute_output = false;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::vector<std::uint32_t> texture_bindings;
    std::vector<std::uint32_t> storage_bindings;
    std::uint32_t uniform_binding = 0;
};

RayTracingBindingState BuildRayTracingBindingState(
    bool use_compute,
    std::size_t texture_count,
    std::size_t storage_count);

} // namespace frame::vulkan
