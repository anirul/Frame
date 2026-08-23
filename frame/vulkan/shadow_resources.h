#pragma once

#include "frame/backend_internal.h"

#include <cstdint>
#include <filesystem>
#include <optional>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame
{
class Logger;
}

namespace frame::vulkan
{

class GpuMemoryManager;
class ShaderCompiler;

class ShadowResources
{
  public:
    static constexpr std::uint32_t kMapSize = 2048;

    ~ShadowResources();

    void CreateResources(
        vk::Device device,
        GpuMemoryManager& gpu_memory_manager,
        vk::Format depth_format);
    void DestroyResources();

    void CreatePipeline(
        vk::Device device,
        const ShaderCompiler& shader_compiler,
        const std::filesystem::path& shader_path,
        const Logger& logger);
    void DestroyPipeline();

    std::optional<vk::DescriptorImageInfo> GetDescriptorInfo() const;

    vk::RenderPass GetRenderPass() const
    {
        return render_pass_ ? *render_pass_ : vk::RenderPass{};
    }
    vk::Framebuffer GetFramebuffer() const
    {
        return framebuffer_ ? *framebuffer_ : vk::Framebuffer{};
    }
    vk::PipelineLayout GetPipelineLayout() const
    {
        return pipeline_layout_ ? *pipeline_layout_ : vk::PipelineLayout{};
    }
    vk::Pipeline GetPipeline() const
    {
        return pipeline_ ? *pipeline_ : vk::Pipeline{};
    }

  private:
    vk::UniqueImage image_;
    vk::UniqueDeviceMemory memory_;
    vk::UniqueImageView view_;
    vk::UniqueSampler sampler_;
    vk::UniqueRenderPass render_pass_;
    vk::UniqueFramebuffer framebuffer_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline pipeline_;
};

} // namespace frame::vulkan
