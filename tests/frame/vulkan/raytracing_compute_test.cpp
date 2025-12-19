#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#ifndef VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1
#endif

#include <gtest/gtest.h>
#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <vector>

#include "frame/file/file_system.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/raytracing_bindings.h"
#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

namespace test
{
namespace
{

struct alignas(16) UniformBlock
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 projection_inv;
    glm::mat4 view_inv;
    glm::mat4 model;
    glm::mat4 model_inv;
    glm::vec4 camera_position;
    glm::vec4 light_dir;
    glm::vec4 light_color;
    glm::vec4 time_s;
};

struct alignas(16) Vertex
{
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 uv_pad;
};

struct alignas(16) Triangle
{
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

struct alignas(16) BvhNode
{
    glm::vec4 min;
    glm::vec4 max;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

vk::ShaderModule CompileShader(const std::filesystem::path& path, vk::Device device)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Unable to open shader file: " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string src = ss.str();
    shaderc::Compiler compiler;
    shaderc::CompileOptions opts;
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(
        src, shaderc_compute_shader, path.string().c_str(), "main", opts);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }
    std::vector<std::uint32_t> code(result.cbegin(), result.cend());
    vk::ShaderModuleCreateInfo info(
        vk::ShaderModuleCreateFlags{},
        code.size() * sizeof(std::uint32_t),
        code.data());
    return device.createShaderModule(info);
}

vk::UniqueSampler MakeSampler(vk::Device device)
{
    vk::SamplerCreateInfo info(
        vk::SamplerCreateFlags{},
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eFloatOpaqueBlack,
        VK_FALSE);
    return device.createSamplerUnique(info);
}

} // namespace

class VulkanRayTracingComputeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto vk_get_instance_proc_addr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr);
        ASSERT_NE(vk_get_instance_proc_addr, nullptr);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

        vk::ApplicationInfo app_info(
            "RayTracingComputeTest",
            1,
            "Frame",
            1,
            VK_API_VERSION_1_1);
        vk::InstanceCreateInfo instance_info({}, &app_info);
        instance_ = vk::createInstanceUnique(instance_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

        auto physical_devices = instance_->enumeratePhysicalDevices();
        ASSERT_FALSE(physical_devices.empty());
        physical_device_ = physical_devices.front();
        if (physical_device_.getProperties().deviceType ==
            vk::PhysicalDeviceType::eCpu)
        {
            GTEST_SKIP() << "Skipping on CPU Vulkan device.";
        }

        auto format_props = physical_device_.getFormatProperties(
            vk::Format::eR16G16B16A16Sfloat);
        if (!(format_props.optimalTilingFeatures &
              vk::FormatFeatureFlagBits::eStorageImage))
        {
            GTEST_SKIP() << "Device does not support rgba16f storage images.";
        }

        const auto queue_props = physical_device_.getQueueFamilyProperties();
        std::optional<std::uint32_t> graphics_index;
        for (std::uint32_t i = 0; i < queue_props.size(); ++i)
        {
            if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                graphics_index = i;
                break;
            }
        }
        ASSERT_TRUE(graphics_index.has_value());
        graphics_family_index_ = graphics_index.value();

        float priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info(
            {}, graphics_family_index_, 1, &priority);
        const auto supported = physical_device_.getFeatures();
        vk::PhysicalDeviceFeatures features{};
        features.geometryShader = supported.geometryShader;
        features.shaderStorageImageExtendedFormats =
            supported.shaderStorageImageExtendedFormats;
        if (!features.shaderStorageImageExtendedFormats)
        {
            GTEST_SKIP()
                << "shaderStorageImageExtendedFormats not supported on device.";
        }
        vk::DeviceCreateInfo device_info({}, 1, &queue_info);
        device_info.setPEnabledFeatures(&features);
        device_ = physical_device_.createDeviceUnique(device_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*device_);

        queue_ = device_->getQueue(graphics_family_index_, 0);
        vk::CommandPoolCreateInfo pool_info(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            graphics_family_index_);
        command_pool_ = device_->createCommandPoolUnique(pool_info);

        memory_manager_ = std::make_unique<frame::vulkan::GpuMemoryManager>(
            physical_device_, *device_);
        command_queue_ = std::make_unique<frame::vulkan::CommandQueue>(
            *device_, queue_, *command_pool_);
    }

    void TearDown() override
    {
        if (device_)
        {
            device_->waitIdle();
        }
    }

    struct ImageResource
    {
        vk::UniqueImage image;
        vk::UniqueDeviceMemory memory;
        vk::UniqueImageView view;
        vk::UniqueSampler sampler;
    };

    ImageResource CreateTexture2D(
        const std::vector<float>& pixels,
        vk::Format format = vk::Format::eR32G32B32A32Sfloat)
    {
        const std::uint32_t width = 1;
        const std::uint32_t height = 1;
        const std::size_t byte_count =
            static_cast<std::size_t>(width) * height * sizeof(glm::vec4);

        vk::ImageCreateInfo image_info(
            vk::ImageCreateFlags{},
            vk::ImageType::e2D,
            format,
            {width, height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        ImageResource res{};
        res.image = device_->createImageUnique(image_info);
        auto requirements = device_->getImageMemoryRequirements(*res.image);
        vk::MemoryAllocateInfo allocate_info(
            requirements.size,
            memory_manager_->FindMemoryType(
                requirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eDeviceLocal));
        res.memory = device_->allocateMemoryUnique(allocate_info);
        device_->bindImageMemory(*res.image, *res.memory, 0);

        // Upload.
        vk::UniqueDeviceMemory staging_memory;
        auto staging = memory_manager_->CreateBuffer(
            byte_count,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_memory);
        void* mapped = device_->mapMemory(
            *staging_memory, 0, byte_count);
        std::memcpy(mapped, pixels.data(), byte_count);
        device_->unmapMemory(*staging_memory);

        command_queue_->TransitionImageLayout(
            *res.image,
            format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal);
        command_queue_->CopyBufferToImage(
            *staging,
            *res.image,
            width,
            height,
            1);
        command_queue_->TransitionImageLayout(
            *res.image,
            format,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::ImageViewCreateInfo view_info(
            vk::ImageViewCreateFlags{},
            *res.image,
            vk::ImageViewType::e2D,
            format,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        res.view = device_->createImageViewUnique(view_info);
        res.sampler = MakeSampler(*device_);
        return res;
    }

    ImageResource CreateCubemap(
        const std::vector<float>& face_pixels,
        vk::Format format = vk::Format::eR32G32B32A32Sfloat)
    {
        const std::uint32_t width = 1;
        const std::uint32_t height = 1;
        const std::size_t byte_count =
            static_cast<std::size_t>(width) * height * sizeof(glm::vec4) * 6;

        vk::ImageCreateInfo image_info(
            vk::ImageCreateFlagBits::eCubeCompatible,
            vk::ImageType::e2D,
            format,
            {width, height, 1},
            1,
            6,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        ImageResource res{};
        res.image = device_->createImageUnique(image_info);
        auto requirements = device_->getImageMemoryRequirements(*res.image);
        vk::MemoryAllocateInfo allocate_info(
            requirements.size,
            memory_manager_->FindMemoryType(
                requirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eDeviceLocal));
        res.memory = device_->allocateMemoryUnique(allocate_info);
        device_->bindImageMemory(*res.image, *res.memory, 0);

        vk::UniqueDeviceMemory staging_memory;
        auto staging = memory_manager_->CreateBuffer(
            byte_count,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_memory);
        void* mapped = device_->mapMemory(
            *staging_memory, 0, byte_count);
        std::memcpy(mapped, face_pixels.data(), byte_count);
        device_->unmapMemory(*staging_memory);

        command_queue_->TransitionImageLayout(
            *res.image,
            format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            6);
        command_queue_->CopyBufferToImage(
            *staging,
            *res.image,
            width,
            height,
            6,
            sizeof(glm::vec4));
        command_queue_->TransitionImageLayout(
            *res.image,
            format,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            6);

        vk::ImageViewCreateInfo view_info(
            vk::ImageViewCreateFlags{},
            *res.image,
            vk::ImageViewType::eCube,
            format,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6});
        res.view = device_->createImageViewUnique(view_info);
        res.sampler = MakeSampler(*device_);
        return res;
    }

    vk::UniqueDeviceMemory MakeBufferWithData(
        const void* data,
        std::size_t bytes,
        vk::UniqueBuffer& out_buffer,
        vk::BufferUsageFlags usage)
    {
        vk::UniqueDeviceMemory staging_memory;
        auto staging = memory_manager_->CreateBuffer(
            bytes,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_memory);
        void* mapped = device_->mapMemory(*staging_memory, 0, bytes);
        std::memcpy(mapped, data, bytes);
        device_->unmapMemory(*staging_memory);

        vk::UniqueDeviceMemory gpu_memory;
        out_buffer = memory_manager_->CreateBuffer(
            bytes,
            vk::BufferUsageFlagBits::eTransferDst | usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            gpu_memory);
        command_queue_->CopyBuffer(*staging, *out_buffer, bytes);
        return gpu_memory;
    }

    vk::UniqueCommandBuffer BeginCommands()
    {
        vk::CommandBufferAllocateInfo alloc_info(
            *command_pool_,
            vk::CommandBufferLevel::ePrimary,
            1);
        auto buffers = device_->allocateCommandBuffersUnique(alloc_info);
        auto cmd = std::move(buffers.front());
        cmd->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return cmd;
    }

    void EndCommands(vk::UniqueCommandBuffer& cmd)
    {
        cmd->end();
        vk::SubmitInfo submit_info(
            0, nullptr, nullptr, 1, &cmd.get(), 0, nullptr);
        queue_.submit(submit_info);
        queue_.waitIdle();
    }

  protected:
    vk::UniqueInstance instance_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vk::Queue queue_;
    vk::UniqueCommandPool command_pool_;
    std::unique_ptr<frame::vulkan::GpuMemoryManager> memory_manager_;
    std::unique_ptr<frame::vulkan::CommandQueue> command_queue_;
    std::uint32_t graphics_family_index_ = 0;
};

TEST_F(VulkanRayTracingComputeTest, DispatchProducesLitPixel)
{
    const auto shader_path = frame::file::FindFile(
        "asset/shader/vulkan/raytracing.comp");
    ASSERT_FALSE(shader_path.empty());
    vk::ShaderModule compute_module =
        CompileShader(shader_path, *device_);

    // Textures.
    std::vector<float> red_rgba = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> normal_rgba = {0.5f, 0.5f, 1.0f, 1.0f};
    std::vector<float> roughness_rgba = {0.05f, 0.0f, 0.0f, 1.0f};
    std::vector<float> metallic_rgba = {0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> ao_rgba = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> sky_rgba(6 * 4, 0.0f); // 6 faces, 1 texel each.

    ImageResource albedo_tex = CreateTexture2D(red_rgba);
    ImageResource normal_tex = CreateTexture2D(normal_rgba);
    ImageResource roughness_tex = CreateTexture2D(roughness_rgba);
    ImageResource metallic_tex = CreateTexture2D(metallic_rgba);
    ImageResource ao_tex = CreateTexture2D(ao_rgba);
    ImageResource skybox_tex = CreateCubemap(sky_rgba);

    // Output image (storage + sampled).
    vk::ImageCreateInfo out_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        vk::Format::eR16G16B16A16Sfloat,
        {1, 1, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);
    vk::UniqueImage out_image = device_->createImageUnique(out_info);
    auto out_req = device_->getImageMemoryRequirements(*out_image);
    vk::MemoryAllocateInfo out_alloc(
        out_req.size,
        memory_manager_->FindMemoryType(
            out_req.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    vk::UniqueDeviceMemory out_memory =
        device_->allocateMemoryUnique(out_alloc);
    device_->bindImageMemory(*out_image, *out_memory, 0);
    vk::ImageViewCreateInfo out_view_info(
        vk::ImageViewCreateFlags{},
        *out_image,
        vk::ImageViewType::e2D,
        vk::Format::eR16G16B16A16Sfloat,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    vk::UniqueImageView out_view =
        device_->createImageViewUnique(out_view_info);
    vk::UniqueSampler out_sampler = MakeSampler(*device_);

    command_queue_->TransitionImageLayout(
        *out_image,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    // Geometry buffers.
    Triangle tri{};
    tri.v0.position = glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
    tri.v1.position = glm::vec4(0.5f, -0.5f, 0.0f, 1.0f);
    tri.v2.position = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
    tri.v0.normal = tri.v1.normal = tri.v2.normal = glm::vec4(0, 0, 1, 0);
    tri.v0.uv_pad = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    tri.v1.uv_pad = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    tri.v2.uv_pad = glm::vec4(0.5f, 1.0f, 0.0f, 0.0f);

    BvhNode node{};
    node.min = glm::vec4(-1.0f, -1.0f, -0.1f, 0.0f);
    node.max = glm::vec4(1.0f, 1.0f, 0.1f, 0.0f);
    node.left = -1;
    node.right = -1;
    node.first_triangle = 0;
    node.triangle_count = 1;

    vk::UniqueBuffer tri_buffer;
    auto tri_memory = MakeBufferWithData(
        &tri,
        sizeof(Triangle),
        tri_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);
    vk::UniqueBuffer bvh_buffer;
    auto bvh_memory = MakeBufferWithData(
        &node,
        sizeof(BvhNode),
        bvh_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);

    // Uniform buffer.
    UniformBlock ubo{};
    const glm::vec3 eye(0.0f, 0.0f, 1.5f);
    ubo.projection = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.projection_inv = glm::mat4(1.0f);
    ubo.view_inv = glm::mat4(1.0f);
    ubo.model = glm::mat4(1.0f);
    ubo.model_inv = glm::mat4(1.0f);
    ubo.camera_position = glm::vec4(eye, 1.0f);
    ubo.light_dir = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    ubo.light_color = glm::vec4(8.0f);
    ubo.time_s = glm::vec4(0.0f);
    vk::UniqueDeviceMemory uniform_memory;
    vk::UniqueBuffer uniform_buffer;
    {
        vk::BufferCreateInfo buf_info(
            {},
            sizeof(UniformBlock),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::SharingMode::eExclusive);
        uniform_buffer = device_->createBufferUnique(buf_info);
        auto req = device_->getBufferMemoryRequirements(*uniform_buffer);
        vk::MemoryAllocateInfo alloc(
            req.size,
            memory_manager_->FindMemoryType(
                req.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent));
        uniform_memory = device_->allocateMemoryUnique(alloc);
        device_->bindBufferMemory(*uniform_buffer, *uniform_memory, 0);
        void* mapped = device_->mapMemory(
            *uniform_memory, 0, sizeof(UniformBlock));
        std::memcpy(mapped, &ubo, sizeof(UniformBlock));
        device_->unmapMemory(*uniform_memory);
    }

    // Descriptor set layout + pipeline.
    const auto binding_state =
        frame::vulkan::BuildRayTracingBindingState(true, 6, 2);
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(binding_state.bindings.size()),
        binding_state.bindings.data());
    vk::UniqueDescriptorSetLayout set_layout =
        device_->createDescriptorSetLayoutUnique(layout_info);

    std::vector<vk::DescriptorPoolSize> pool_sizes = {
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eCombinedImageSampler, 8},
        {vk::DescriptorType::eStorageBuffer, 2},
        {vk::DescriptorType::eUniformBuffer, 1},
    };
    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlags{},
        1,
        static_cast<std::uint32_t>(pool_sizes.size()),
        pool_sizes.data());
    vk::UniqueDescriptorPool descriptor_pool =
        device_->createDescriptorPoolUnique(pool_info);

    vk::DescriptorSetAllocateInfo alloc_info(
        *descriptor_pool, 1, &set_layout.get());
    auto sets = device_->allocateDescriptorSetsUnique(alloc_info);
    vk::DescriptorSet descriptor_set = sets.front().get();

    // Descriptor writes.
    std::vector<vk::WriteDescriptorSet> writes;
    vk::DescriptorImageInfo storage_image_info(
        nullptr, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        0,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    // Output sample binding uses output image sampler for completeness.
    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        1,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<ImageResource*, 6> textures = {
        &albedo_tex,
        &normal_tex,
        &roughness_tex,
        &metallic_tex,
        &ao_tex,
        &skybox_tex};
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (std::size_t i = 0; i < textures.size(); ++i)
    {
        texture_infos.emplace_back(
            *textures[i]->sampler,
            *textures[i]->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            static_cast<std::uint32_t>(binding_state.texture_bindings[i]),
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo tri_info(*tri_buffer, 0, sizeof(Triangle));
    writes.emplace_back(
        descriptor_set,
        binding_state.storage_bindings[0],
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &tri_info);
    vk::DescriptorBufferInfo bvh_info(*bvh_buffer, 0, sizeof(BvhNode));
    writes.emplace_back(
        descriptor_set,
        binding_state.storage_bindings[1],
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &bvh_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        binding_state.uniform_binding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &uniform_info);
    device_->updateDescriptorSets(
        static_cast<std::uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr);

    vk::PipelineLayoutCreateInfo layout_ci(
        {}, 1, &set_layout.get(), 0, nullptr);
    vk::UniquePipelineLayout pipeline_layout =
        device_->createPipelineLayoutUnique(layout_ci);

    vk::PipelineShaderStageCreateInfo stage_info(
        vk::PipelineShaderStageCreateFlags{},
        vk::ShaderStageFlagBits::eCompute,
        compute_module,
        "main");
    vk::ComputePipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        stage_info,
        *pipeline_layout);
    auto pipeline_result =
        device_->createComputePipelineUnique(nullptr, pipeline_info);
    ASSERT_EQ(pipeline_result.result, vk::Result::eSuccess);
    vk::UniquePipeline pipeline = std::move(pipeline_result.value);

    // Dispatch.
    auto cmd = BeginCommands();
    cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipeline_layout,
        0,
        descriptor_set,
        {});
    cmd->dispatch(1, 1, 1);

    // Make output readable.
    vk::ImageMemoryBarrier to_transfer(
        vk::AccessFlagBits::eShaderWrite,
        vk::AccessFlagBits::eTransferRead,
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eTransferSrcOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        *out_image,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    cmd->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eTransfer,
        {},
        nullptr,
        nullptr,
        to_transfer);

    const vk::DeviceSize copy_size = sizeof(std::uint16_t) * 4;
    vk::UniqueDeviceMemory readback_memory;
    auto readback = memory_manager_->CreateBuffer(
        copy_size,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        readback_memory);
    vk::BufferImageCopy region(
        0,
        0,
        0,
        {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        {0, 0, 0},
        {1, 1, 1});
    cmd->copyImageToBuffer(
        *out_image,
        vk::ImageLayout::eTransferSrcOptimal,
        *readback,
        region);
    EndCommands(cmd);

    const auto* raw16 = static_cast<const std::uint16_t*>(
        device_->mapMemory(*readback_memory, 0, copy_size));
    glm::vec2 rg = glm::unpackHalf2x16(
        (static_cast<std::uint32_t>(raw16[1]) << 16) | raw16[0]);
    glm::vec2 ba = glm::unpackHalf2x16(
        (static_cast<std::uint32_t>(raw16[3]) << 16) | raw16[2]);
    device_->unmapMemory(*readback_memory);

    EXPECT_TRUE(std::isfinite(rg.x));
    EXPECT_TRUE(std::isfinite(rg.y));
    EXPECT_TRUE(std::isfinite(ba.x));
    const float max_rgb = std::max(rg.x, std::max(rg.y, ba.x));
    EXPECT_GT(max_rgb, 0.2f);   // Pixel should be lit.
    EXPECT_GE(ba.y, 0.9f);      // Alpha close to 1
    device_->destroyShaderModule(compute_module);
}

} // namespace test
