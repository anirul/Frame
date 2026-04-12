#include "frame/vulkan/pipeline_resources.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "frame/vulkan/device.h"

#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/output_image_resources.h"
#include "frame/vulkan/shader_compiler.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/texture_resources.h"

namespace frame::vulkan
{

namespace
{

template <typename T>
T AlignUp(T value, T alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

PipelineResources::PipelineResources(Device& device) : device_(device)
{
}

void PipelineResources::CreateGraphicsPipeline()
{
    if (!device_.vk_unique_device_ || !device_.swapchain_resources_ ||
        !device_.swapchain_resources_->IsValid())
    {
        return;
    }

    const auto& render_pass = device_.swapchain_resources_->GetRenderPass();

    DestroyGraphicsPipeline();

    if (!device_.texture_resources_ || device_.texture_resources_->Empty())
    {
        return;
    }

    std::vector<std::uint32_t> vert_code;
    std::vector<std::uint32_t> frag_code;
    if (!device_.active_program_info_)
    {
        device_.logger_->error(
            "No Vulkan program selected; cannot build graphics pipeline.");
        return;
    }
    if (device_.active_program_info_->vertex_shader.empty() ||
        device_.active_program_info_->fragment_shader.empty())
    {
        device_.logger_->error(
            "Vulkan program {} is missing shader filenames.",
            device_.active_program_info_->program_name);
        return;
    }

    if (!device_.shader_compiler_)
    {
        device_.shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    try
    {
        vert_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->vertex_shader,
            shaderc_vertex_shader);
        frag_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->fragment_shader,
            shaderc_fragment_shader);
    }
    catch (const std::exception& ex)
    {
        device_.logger_->warn(
            "Failed to compile Vulkan shader pair ({} / {}): {}",
            device_.active_program_info_->vertex_shader.string(),
            device_.active_program_info_->fragment_shader.string(),
            ex.what());
        return;
    }

    use_procedural_quad_pipeline_ =
        device_.active_program_info_->scene_type ==
        frame::proto::SceneType::QUAD;

    struct alignas(16) PushConstants
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        float time_s;
    };

    if (use_procedural_quad_pipeline_)
    {
        if (device_.active_program_info_->uses_time_uniform)
        {
            push_constant_size_ = sizeof(float);
            push_constant_stages_ = vk::ShaderStageFlagBits::eFragment;
        }
        else
        {
            push_constant_size_ = 0;
            push_constant_stages_ = {};
        }
    }
    else
    {
        push_constant_size_ = static_cast<std::uint32_t>(
            sizeof(PushConstants));
        push_constant_stages_ = vk::ShaderStageFlagBits::eVertex;
    }

    auto vert_module = CreateShaderModule(vert_code);
    auto frag_module = CreateShaderModule(frag_code);

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eVertex,
         *vert_module,
         "main"},
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eFragment,
         *frag_module,
         "main"},
    };

    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

    if (!use_procedural_quad_pipeline_)
    {
        binding_descriptions.emplace_back(
            0,
            static_cast<std::uint32_t>(sizeof(MeshVertex)),
            vk::VertexInputRate::eVertex);
        attribute_descriptions.emplace_back(
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, position)));
        attribute_descriptions.emplace_back(
            1,
            0,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, uv)));
    }

    vk::PipelineVertexInputStateCreateInfo vertex_input_info(
        vk::PipelineVertexInputStateCreateFlags{},
        static_cast<std::uint32_t>(binding_descriptions.size()),
        binding_descriptions.data(),
        static_cast<std::uint32_t>(attribute_descriptions.size()),
        attribute_descriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo input_assembly(
        vk::PipelineInputAssemblyStateCreateFlags{},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE);

    vk::PipelineViewportStateCreateInfo viewport_state(
        vk::PipelineViewportStateCreateFlags{},
        1,
        nullptr,
        1,
        nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags{},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags{},
        vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo color_blending(
        vk::PipelineColorBlendStateCreateFlags{},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1,
        &color_blend_attachment);

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state(
        vk::PipelineDynamicStateCreateFlags{},
        static_cast<std::uint32_t>(dynamic_states.size()),
        dynamic_states.data());

    std::vector<vk::DescriptorSetLayout> set_layouts;
    if (device_.descriptor_set_layout_)
    {
        set_layouts.push_back(*device_.descriptor_set_layout_);
    }

    std::vector<vk::PushConstantRange> push_constant_ranges;
    if (push_constant_size_ > 0)
    {
        push_constant_ranges.emplace_back(
            push_constant_stages_,
            0,
            push_constant_size_);
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info(
        vk::PipelineLayoutCreateFlags{},
        static_cast<std::uint32_t>(set_layouts.size()),
        set_layouts.data(),
        static_cast<std::uint32_t>(push_constant_ranges.size()),
        push_constant_ranges.empty() ? nullptr : push_constant_ranges.data());
    pipeline_layout_ =
        device_.vk_unique_device_->createPipelineLayoutUnique(
            pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        2,
        shader_stages,
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterizer,
        &multisampling,
        nullptr,
        &color_blending,
        &dynamic_state,
        *pipeline_layout_,
        *render_pass);

    auto pipeline_result =
        device_.vk_unique_device_->createGraphicsPipelineUnique(
            nullptr,
            pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to create Vulkan graphics pipeline.");
    }
    graphics_pipeline_ = std::move(pipeline_result.value);
}

void PipelineResources::DestroyGraphicsPipeline()
{
    graphics_pipeline_.reset();
    pipeline_layout_.reset();
    use_procedural_quad_pipeline_ = false;
    push_constant_stages_ = {};
    push_constant_size_ = 0;
}

void PipelineResources::CreateComputePipeline()
{
    if (!device_.use_compute_raytracing_ || !device_.vk_unique_device_)
    {
        return;
    }

    DestroyComputePipeline();

    if (!device_.descriptor_set_layout_)
    {
        device_.logger_->warn(
            "CreateComputePipeline skipped: descriptor set layout missing.");
        return;
    }

    if (!device_.active_program_info_ ||
        device_.active_program_info_->compute_shader.empty())
    {
        device_.logger_->warn(
            "CreateComputePipeline skipped: no active compute shader.");
        return;
    }

    if (!device_.shader_compiler_)
    {
        device_.shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    std::vector<std::uint32_t> compute_code;
    try
    {
        compute_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->compute_shader,
            shaderc_compute_shader);
    }
    catch (const std::exception& ex)
    {
        device_.logger_->warn(
            "Failed to compile compute shader {}: {}",
            device_.active_program_info_->compute_shader.string(),
            ex.what());
        return;
    }

    auto compute_module = CreateShaderModule(compute_code);

    vk::PipelineShaderStageCreateInfo stage_info(
        vk::PipelineShaderStageCreateFlags{},
        vk::ShaderStageFlagBits::eCompute,
        *compute_module,
        "main");

    const vk::DescriptorSetLayout layouts[] = {*device_.descriptor_set_layout_};
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{},
        1,
        layouts);
    compute_pipeline_layout_ =
        device_.vk_unique_device_->createPipelineLayoutUnique(layout_info);

    vk::ComputePipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        stage_info,
        *compute_pipeline_layout_);

    auto pipeline_result =
        device_.vk_unique_device_->createComputePipelineUnique(
            nullptr,
            pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to create Vulkan compute pipeline.");
    }
    compute_pipeline_ = std::move(pipeline_result.value);
    device_.logger_->info("Created raytracing compute pipeline.");
}

void PipelineResources::DestroyComputePipeline()
{
    compute_pipeline_.reset();
    compute_pipeline_layout_.reset();
    if (device_.output_image_resources_)
    {
        device_.output_image_resources_->SetComputeOutputInShaderRead(false);
    }
}

void PipelineResources::CreateRaytracingPipeline()
{
    if (!device_.use_raytracing_pipeline_ || !device_.vk_unique_device_)
    {
        return;
    }

    DestroyRaytracingPipeline();

    if (!device_.descriptor_set_layout_)
    {
        device_.logger_->warn(
            "CreateRaytracingPipeline skipped: descriptor set layout missing.");
        return;
    }
    if (!device_.active_program_info_ ||
        device_.active_program_info_->raygen_shader.empty() ||
        device_.active_program_info_->miss_shader.empty() ||
        device_.active_program_info_->closesthit_shader.empty())
    {
        device_.logger_->warn(
            "CreateRaytracingPipeline skipped: missing ray tracing shader "
            "stage.");
        return;
    }
    if (!device_.shader_compiler_)
    {
        device_.shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    std::vector<std::uint32_t> raygen_code;
    std::vector<std::uint32_t> miss_code;
    std::vector<std::uint32_t> closesthit_code;
    try
    {
        raygen_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->raygen_shader,
            shaderc_raygen_shader);
        miss_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->miss_shader,
            shaderc_miss_shader);
        closesthit_code = device_.shader_compiler_->CompileFile(
            device_.active_program_info_->closesthit_shader,
            shaderc_closesthit_shader);
    }
    catch (const std::exception& ex)
    {
        device_.logger_->warn(
            "Failed to compile Vulkan ray tracing shaders ({}, {}, {}): {}",
            device_.active_program_info_->raygen_shader.string(),
            device_.active_program_info_->miss_shader.string(),
            device_.active_program_info_->closesthit_shader.string(),
            ex.what());
        return;
    }

    auto raygen_module = CreateShaderModule(raygen_code);
    auto miss_module = CreateShaderModule(miss_code);
    auto closesthit_module = CreateShaderModule(closesthit_code);

    std::array<vk::PipelineShaderStageCreateInfo, 3> shader_stages = { {
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eRaygenKHR,
         *raygen_module,
         "main"},
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eMissKHR,
         *miss_module,
         "main"},
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eClosestHitKHR,
         *closesthit_module,
         "main"},
    } };

    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> shader_groups = { {
        {vk::RayTracingShaderGroupTypeKHR::eGeneral,
         0,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
        {vk::RayTracingShaderGroupTypeKHR::eGeneral,
         1,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
        {vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
         VK_SHADER_UNUSED_KHR,
         2,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
    } };

    const vk::DescriptorSetLayout layouts[] = {*device_.descriptor_set_layout_};
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{},
        1,
        layouts);
    raytracing_pipeline_layout_ =
        device_.vk_unique_device_->createPipelineLayoutUnique(layout_info);

    vk::RayTracingPipelineCreateInfoKHR pipeline_info{};
    pipeline_info.setStages(shader_stages);
    pipeline_info.setGroups(shader_groups);
    pipeline_info.setMaxPipelineRayRecursionDepth(1);
    pipeline_info.setLayout(*raytracing_pipeline_layout_);
    auto pipeline_result =
        device_.vk_unique_device_->createRayTracingPipelineKHRUnique(
            vk::DeferredOperationKHR{},
            vk::PipelineCache{},
            pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to create Vulkan ray tracing pipeline.");
    }
    raytracing_pipeline_ = std::move(pipeline_result.value);

    const std::uint32_t handle_size =
        device_.raytracing_pipeline_properties_.shaderGroupHandleSize;
    const std::uint32_t handle_alignment =
        device_.raytracing_pipeline_properties_.shaderGroupHandleAlignment;
    const std::uint32_t base_alignment =
        device_.raytracing_pipeline_properties_.shaderGroupBaseAlignment;
    const std::uint32_t handle_size_aligned =
        AlignUp(handle_size, handle_alignment);
    const std::uint32_t raygen_stride =
        AlignUp(handle_size_aligned, base_alignment);
    const std::uint32_t miss_stride = handle_size_aligned;
    const std::uint32_t hit_stride = handle_size_aligned;
    const std::uint32_t raygen_size = raygen_stride;
    const std::uint32_t miss_size =
        AlignUp(handle_size_aligned, base_alignment);
    const std::uint32_t hit_size =
        AlignUp(handle_size_aligned, base_alignment);
    const vk::DeviceSize sbt_size =
        static_cast<vk::DeviceSize>(raygen_size + miss_size + hit_size);

    std::vector<std::uint8_t> shader_handles(
        static_cast<std::size_t>(handle_size * shader_groups.size()));
    const vk::Result handle_result =
        device_.vk_unique_device_->getRayTracingShaderGroupHandlesKHR(
            *raytracing_pipeline_,
            0,
            static_cast<std::uint32_t>(shader_groups.size()),
            shader_handles.size(),
            shader_handles.data());
    if (handle_result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to query Vulkan ray tracing shader group handles.");
    }

    raytracing_sbt_buffer_ = device_.gpu_memory_manager_->CreateBuffer(
        sbt_size,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        raytracing_sbt_memory_,
        vk::MemoryAllocateFlagBits::eDeviceAddress);
    auto* mapped = static_cast<std::uint8_t*>(device_.vk_unique_device_->mapMemory(
        *raytracing_sbt_memory_,
        0,
        sbt_size));
    std::memset(mapped, 0, static_cast<std::size_t>(sbt_size));
    std::memcpy(mapped, shader_handles.data(), handle_size);
    std::memcpy(
        mapped + raygen_size,
        shader_handles.data() + handle_size,
        handle_size);
    std::memcpy(
        mapped + raygen_size + miss_size,
        shader_handles.data() + handle_size * 2,
        handle_size);
    device_.vk_unique_device_->unmapMemory(*raytracing_sbt_memory_);

    const vk::DeviceAddress sbt_address =
        device_.vk_unique_device_->getBufferAddress(
            vk::BufferDeviceAddressInfo(*raytracing_sbt_buffer_));
    raygen_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address,
        raygen_stride,
        raygen_size);
    miss_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address + raygen_size,
        miss_stride,
        miss_size);
    hit_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address + raygen_size + miss_size,
        hit_stride,
        hit_size);
    callable_sbt_region_ = vk::StridedDeviceAddressRegionKHR();

    device_.logger_->info("Created Vulkan ray tracing pipeline.");
}

void PipelineResources::DestroyRaytracingPipeline()
{
    raytracing_pipeline_.reset();
    raytracing_pipeline_layout_.reset();
    raytracing_sbt_buffer_.reset();
    raytracing_sbt_memory_.reset();
    raygen_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    miss_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    hit_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    callable_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    if (device_.output_image_resources_)
    {
        device_.output_image_resources_->SetComputeOutputInShaderRead(false);
    }
}

vk::UniqueShaderModule PipelineResources::CreateShaderModule(
    const std::vector<std::uint32_t>& code) const
{
    vk::ShaderModuleCreateInfo create_info(
        vk::ShaderModuleCreateFlags{},
        code.size() * sizeof(std::uint32_t),
        code.data());
    return device_.vk_unique_device_->createShaderModuleUnique(create_info);
}

} // namespace frame::vulkan
