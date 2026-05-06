#include "frame/vulkan/raytrace_scene_renderer.h"

#include <vector>

#include "frame/vulkan/device.h"

#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/frame_profiler.h"
#include "frame/vulkan/output_image_resources.h"
#include "frame/vulkan/pipeline_resources.h"
#include "frame/vulkan/scene_state.h"

namespace frame::vulkan
{

RaytraceSceneRenderer::RaytraceSceneRenderer(Device& device) : device_(device)
{
}

void RaytraceSceneRenderer::UpdateUniformBuffer(const SceneState& state)
{
    VulkanProfileScope scope("raytrace.update_uniform_buffer");

    if (!device_.buffer_resources_)
    {
        return;
    }
    if (!device_.buffer_resources_->GetUniformBuffer())
    {
        return;
    }
    auto block = MakeUniformBlock(state, device_.elapsed_time_seconds_);
    device_.buffer_resources_->UpdateUniform(&block, sizeof(UniformBlock));
}

void RaytraceSceneRenderer::Render(
    vk::CommandBuffer command_buffer,
    vk::Extent2D extent)
{
    VulkanProfileScope scope("raytrace.render_record");

    auto transition_output = [&](vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout,
                                 vk::PipelineStageFlags src_stage,
                                 vk::PipelineStageFlags dst_stage,
                                 vk::AccessFlags src_access,
                                 vk::AccessFlags dst_access) {
        if (!device_.output_image_resources_ ||
            !device_.output_image_resources_->HasComputeOutputImage())
        {
            return;
        }
        vk::ImageMemoryBarrier barrier(
            src_access,
            dst_access,
            old_layout,
            new_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            device_.output_image_resources_->GetComputeOutputImage(),
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            nullptr,
            nullptr,
            barrier);
    };

    if (!device_.use_compute_raytracing_ || !device_.descriptor_set_ ||
        !device_.output_image_resources_ ||
        !device_.output_image_resources_->HasComputeOutputImage() ||
        extent.width == 0 || extent.height == 0)
    {
        return;
    }

    if (!device_.storage_buffers_ready_ && device_.buffer_resources_)
    {
        VulkanProfileScope barrier_scope(
            "raytrace.storage_buffer_barrier_record");
        const auto& storage_buffers =
            device_.buffer_resources_->GetStorageBuffers();
        std::vector<vk::BufferMemoryBarrier> buffer_barriers;
        buffer_barriers.reserve(storage_buffers.size());
        for (const auto& storage : storage_buffers)
        {
            if (!storage.buffer || storage.size == 0)
            {
                continue;
            }
            buffer_barriers.emplace_back(
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *storage.buffer,
                0,
                storage.size);
        }
        if (!buffer_barriers.empty())
        {
            command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                device_.use_raytracing_pipeline_
                    ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                    : vk::PipelineStageFlagBits::eComputeShader,
                {},
                nullptr,
                buffer_barriers,
                nullptr);
        }
        device_.storage_buffers_ready_ = true;
    }

    if (device_.output_image_resources_->IsComputeOutputInShaderRead())
    {
        VulkanProfileScope transition_scope(
            "raytrace.output_to_general_record");
        transition_output(
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits::eFragmentShader,
            device_.use_raytracing_pipeline_
                ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                : vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderRead,
            vk::AccessFlagBits::eShaderWrite);
        device_.output_image_resources_->SetComputeOutputInShaderRead(false);
    }

    if (device_.use_raytracing_pipeline_ && device_.pipeline_resources_ &&
        device_.pipeline_resources_->HasRaytracingPipeline() &&
        device_.pipeline_resources_->GetRaytracingPipelineLayout())
    {
        VulkanProfileScope dispatch_scope("raytrace.trace_rays_record");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eRayTracingKHR,
            device_.pipeline_resources_->GetRaytracingPipeline());
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            device_.pipeline_resources_->GetRaytracingPipelineLayout(),
            0,
            device_.descriptor_set_,
            {});
        command_buffer.traceRaysKHR(
            device_.pipeline_resources_->GetRaygenSbtRegion(),
            device_.pipeline_resources_->GetMissSbtRegion(),
            device_.pipeline_resources_->GetHitSbtRegion(),
            device_.pipeline_resources_->GetCallableSbtRegion(),
            extent.width,
            extent.height,
            1);
        RecordVulkanProfileCounter("raytrace.trace_rays_dispatches");
    }
    else if (device_.pipeline_resources_ &&
             device_.pipeline_resources_->HasComputePipeline() &&
             device_.pipeline_resources_->GetComputePipelineLayout())
    {
        VulkanProfileScope dispatch_scope("raytrace.compute_dispatch_record");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            device_.pipeline_resources_->GetComputePipeline());
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            device_.pipeline_resources_->GetComputePipelineLayout(),
            0,
            device_.descriptor_set_,
            {});
        const std::uint32_t group_x = (extent.width + 7) / 8;
        const std::uint32_t group_y = (extent.height + 7) / 8;
        command_buffer.dispatch(group_x, group_y, 1);
        RecordVulkanProfileCounter("raytrace.compute_dispatches");
    }

    {
        VulkanProfileScope transition_scope(
            "raytrace.output_to_shader_read_record");
        transition_output(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            device_.use_raytracing_pipeline_
                ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                : vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
    }
    device_.output_image_resources_->SetComputeOutputInShaderRead(true);
}

} // namespace frame::vulkan
