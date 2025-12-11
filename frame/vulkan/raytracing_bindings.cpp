#include "frame/vulkan/raytracing_bindings.h"

namespace frame::vulkan
{

RayTracingBindingState BuildRayTracingBindingState(
    bool use_compute,
    std::size_t texture_count,
    std::size_t storage_count)
{
    RayTracingBindingState state{};
    state.has_compute_output = use_compute;

    const std::uint32_t kBindingOutputImage = 0;
    const std::uint32_t kBindingOutputSample = 1;
    const std::uint32_t kBindingTextureBase = use_compute ? 2u : 0u;
    const std::uint32_t kBindingTriangle = 8u;
    const std::uint32_t kBindingBvh = 9u;
    const std::uint32_t kBindingUniform =
        use_compute ? 10u : static_cast<std::uint32_t>(
            kBindingTextureBase + texture_count + storage_count);

    if (state.has_compute_output)
    {
        state.bindings.emplace_back(
            kBindingOutputImage,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute);
        state.bindings.emplace_back(
            kBindingOutputSample,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment |
                vk::ShaderStageFlagBits::eCompute);
    }

    vk::ShaderStageFlags texture_stages = vk::ShaderStageFlagBits::eFragment;
    if (use_compute)
    {
        texture_stages |= vk::ShaderStageFlagBits::eCompute;
    }
    for (std::uint32_t i = 0; i < texture_count; ++i)
    {
        const std::uint32_t binding = kBindingTextureBase + i;
        state.texture_bindings.push_back(binding);
        state.bindings.emplace_back(
            binding,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            texture_stages);
    }

    vk::ShaderStageFlags buffer_stage = use_compute
        ? vk::ShaderStageFlagBits::eCompute
        : vk::ShaderStageFlagBits::eFragment;
    for (std::uint32_t i = 0; i < storage_count; ++i)
    {
        std::uint32_t binding = kBindingTextureBase +
            static_cast<std::uint32_t>(texture_count) + i;
        if (use_compute)
        {
            if (i == 0)
            {
                binding = kBindingTriangle;
            }
            else if (i == 1)
            {
                binding = kBindingBvh;
            }
        }
        state.storage_bindings.push_back(binding);
        state.bindings.emplace_back(
            binding,
            vk::DescriptorType::eStorageBuffer,
            1,
            buffer_stage);
    }

    vk::ShaderStageFlags uniform_stages =
        vk::ShaderStageFlagBits::eFragment |
        vk::ShaderStageFlagBits::eVertex;
    if (use_compute)
    {
        uniform_stages |= vk::ShaderStageFlagBits::eCompute;
    }
    state.uniform_binding = kBindingUniform;
    state.bindings.emplace_back(
        state.uniform_binding,
        vk::DescriptorType::eUniformBuffer,
        1,
        uniform_stages);

    return state;
}

} // namespace frame::vulkan
