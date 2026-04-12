#pragma once

#include <cstdint>
#include <vector>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class Device;

class PipelineResources
{
  public:
    explicit PipelineResources(Device& device);

    void CreateGraphicsPipeline();
    void DestroyGraphicsPipeline();
    void CreateComputePipeline();
    void DestroyComputePipeline();
    void CreateRaytracingPipeline();
    void DestroyRaytracingPipeline();

    bool HasGraphicsPipeline() const
    {
        return static_cast<bool>(graphics_pipeline_);
    }
    bool HasComputePipeline() const
    {
        return static_cast<bool>(compute_pipeline_);
    }
    bool HasRaytracingPipeline() const
    {
        return static_cast<bool>(raytracing_pipeline_);
    }
    vk::Pipeline GetGraphicsPipeline() const
    {
        return graphics_pipeline_.get();
    }
    vk::PipelineLayout GetGraphicsPipelineLayout() const
    {
        return pipeline_layout_.get();
    }
    vk::Pipeline GetComputePipeline() const
    {
        return compute_pipeline_.get();
    }
    vk::PipelineLayout GetComputePipelineLayout() const
    {
        return compute_pipeline_layout_.get();
    }
    vk::Pipeline GetRaytracingPipeline() const
    {
        return raytracing_pipeline_.get();
    }
    vk::PipelineLayout GetRaytracingPipelineLayout() const
    {
        return raytracing_pipeline_layout_.get();
    }
    bool UsesProceduralQuadPipeline() const
    {
        return use_procedural_quad_pipeline_;
    }
    vk::ShaderStageFlags GetPushConstantStages() const
    {
        return push_constant_stages_;
    }
    std::uint32_t GetPushConstantSize() const
    {
        return push_constant_size_;
    }
    const vk::StridedDeviceAddressRegionKHR& GetRaygenSbtRegion() const
    {
        return raygen_sbt_region_;
    }
    const vk::StridedDeviceAddressRegionKHR& GetMissSbtRegion() const
    {
        return miss_sbt_region_;
    }
    const vk::StridedDeviceAddressRegionKHR& GetHitSbtRegion() const
    {
        return hit_sbt_region_;
    }
    const vk::StridedDeviceAddressRegionKHR& GetCallableSbtRegion() const
    {
        return callable_sbt_region_;
    }

  private:
    vk::UniqueShaderModule CreateShaderModule(
        const std::vector<std::uint32_t>& code) const;

  private:
    Device& device_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline graphics_pipeline_;
    vk::UniquePipeline compute_pipeline_;
    vk::UniquePipelineLayout compute_pipeline_layout_;
    vk::UniquePipeline raytracing_pipeline_;
    vk::UniquePipelineLayout raytracing_pipeline_layout_;
    bool use_procedural_quad_pipeline_ = false;
    vk::ShaderStageFlags push_constant_stages_ = {};
    std::uint32_t push_constant_size_ = 0;
    vk::UniqueBuffer raytracing_sbt_buffer_;
    vk::UniqueDeviceMemory raytracing_sbt_memory_;
    vk::StridedDeviceAddressRegionKHR raygen_sbt_region_ = {};
    vk::StridedDeviceAddressRegionKHR miss_sbt_region_ = {};
    vk::StridedDeviceAddressRegionKHR hit_sbt_region_ = {};
    vk::StridedDeviceAddressRegionKHR callable_sbt_region_ = {};
};

} // namespace frame::vulkan
