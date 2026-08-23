#include "frame/vulkan/shadow_resources.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <shaderc/shaderc.h>

#include "frame/logger.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/shader_compiler.h"

namespace frame::vulkan
{

ShadowResources::~ShadowResources()
{
    DestroyPipeline();
    DestroyResources();
}

void ShadowResources::CreateResources(
    vk::Device device,
    GpuMemoryManager& gpu_memory_manager,
    vk::Format depth_format)
{
    DestroyResources();

    vk::AttachmentDescription depth_attachment(
        vk::AttachmentDescriptionFlags{},
        depth_format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::AttachmentReference depth_ref(
        0, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags{},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        0,
        nullptr,
        nullptr,
        &depth_ref);
    std::array<vk::SubpassDependency, 2> dependencies = {
        vk::SubpassDependency(
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eShaderRead,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::DependencyFlagBits::eByRegion),
        vk::SubpassDependency(
            0,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion)};
    vk::RenderPassCreateInfo render_pass_info(
        vk::RenderPassCreateFlags{},
        1,
        &depth_attachment,
        1,
        &subpass,
        static_cast<std::uint32_t>(dependencies.size()),
        dependencies.data());
    render_pass_ = device.createRenderPassUnique(render_pass_info);

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        depth_format,
        vk::Extent3D(kMapSize, kMapSize, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment |
            vk::ImageUsageFlagBits::eSampled);
    image_ = device.createImageUnique(image_info);
    const auto memory_requirements = device.getImageMemoryRequirements(*image_);
    vk::MemoryAllocateInfo alloc_info(
        memory_requirements.size,
        gpu_memory_manager.FindMemoryType(
            memory_requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    memory_ = device.allocateMemoryUnique(alloc_info);
    device.bindImageMemory(*image_, *memory_, 0);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *image_,
        vk::ImageViewType::e2D,
        depth_format,
        {},
        {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    view_ = device.createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eFloatOpaqueWhite,
        VK_FALSE);
    sampler_ = device.createSamplerUnique(sampler_info);

    const vk::ImageView view = *view_;
    vk::FramebufferCreateInfo framebuffer_info(
        vk::FramebufferCreateFlags{},
        *render_pass_,
        1,
        &view,
        kMapSize,
        kMapSize,
        1);
    framebuffer_ = device.createFramebufferUnique(framebuffer_info);
}

void ShadowResources::DestroyResources()
{
    framebuffer_.reset();
    render_pass_.reset();
    sampler_.reset();
    view_.reset();
    image_.reset();
    memory_.reset();
}

std::optional<vk::DescriptorImageInfo> ShadowResources::GetDescriptorInfo()
    const
{
    if (!sampler_ || !view_)
    {
        return std::nullopt;
    }
    return vk::DescriptorImageInfo(
        *sampler_, *view_, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void ShadowResources::CreatePipeline(
    vk::Device device,
    const ShaderCompiler& shader_compiler,
    const std::filesystem::path& shader_path,
    const Logger& logger)
{
    DestroyPipeline();

    std::vector<std::uint32_t> vert_code;
    try
    {
        vert_code =
            shader_compiler.CompileFile(shader_path, shaderc_vertex_shader);
    }
    catch (const std::exception& ex)
    {
        logger->warn("Failed to compile raster shadow shader: {}", ex.what());
        return;
    }

    vk::ShaderModuleCreateInfo shader_module_info(
        vk::ShaderModuleCreateFlags{},
        vert_code.size() * sizeof(std::uint32_t),
        vert_code.data());
    auto vert_module = device.createShaderModuleUnique(shader_module_info);
    vk::PipelineShaderStageCreateInfo shader_stage(
        vk::PipelineShaderStageCreateFlags{},
        vk::ShaderStageFlagBits::eVertex,
        *vert_module,
        "main");

    vk::VertexInputBindingDescription binding_description(
        0,
        static_cast<std::uint32_t>(sizeof(MeshVertex)),
        vk::VertexInputRate::eVertex);
    vk::VertexInputAttributeDescription position_attribute(
        0,
        0,
        vk::Format::eR32G32B32Sfloat,
        static_cast<std::uint32_t>(offsetof(MeshVertex, position)));
    vk::PipelineVertexInputStateCreateInfo vertex_input(
        vk::PipelineVertexInputStateCreateFlags{},
        1,
        &binding_description,
        1,
        &position_attribute);
    vk::PipelineInputAssemblyStateCreateInfo input_assembly(
        vk::PipelineInputAssemblyStateCreateFlags{},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE);
    vk::PipelineViewportStateCreateInfo viewport_state(
        vk::PipelineViewportStateCreateFlags{}, 1, nullptr, 1, nullptr);
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags{},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_TRUE,
        1.25f,
        0.0f,
        1.75f,
        1.0f);
    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags{}, vk::SampleCountFlagBits::e1);
    vk::PipelineDepthStencilStateCreateInfo depth_stencil(
        vk::PipelineDepthStencilStateCreateFlags{},
        VK_TRUE,
        VK_TRUE,
        vk::CompareOp::eLessOrEqual,
        VK_FALSE,
        VK_FALSE);
    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state(
        vk::PipelineDynamicStateCreateFlags{},
        static_cast<std::uint32_t>(dynamic_states.size()),
        dynamic_states.data());

    struct alignas(16) ShadowPushConstants
    {
        glm::mat4 light_view_projection;
        glm::mat4 model;
    };
    vk::PushConstantRange push_constant_range(
        vk::ShaderStageFlagBits::eVertex,
        0,
        static_cast<std::uint32_t>(sizeof(ShadowPushConstants)));
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{}, 0, nullptr, 1, &push_constant_range);
    pipeline_layout_ = device.createPipelineLayoutUnique(layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        1,
        &shader_stage,
        &vertex_input,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterizer,
        &multisampling,
        &depth_stencil,
        nullptr,
        &dynamic_state,
        *pipeline_layout_,
        *render_pass_);
    auto pipeline_result =
        device.createGraphicsPipelineUnique(nullptr, pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create Vulkan shadow pipeline.");
    }
    pipeline_ = std::move(pipeline_result.value);
}

void ShadowResources::DestroyPipeline()
{
    pipeline_.reset();
    pipeline_layout_.reset();
}

} // namespace frame::vulkan
