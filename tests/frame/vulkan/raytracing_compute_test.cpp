#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1

#include <gtest/gtest.h>
#include <shaderc/shaderc.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <utility>
#include <vector>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/logger.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/vulkan_dispatch.h"
#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

namespace test
{
namespace
{

using UniformBlock = frame::vulkan::UniformBlock;

constexpr std::size_t kTriangleVec4Count = 9;

constexpr std::uint32_t kBindingOutputImage = 0;
constexpr std::uint32_t kBindingOutputSampler = 1;
constexpr std::uint32_t kBindingAlbedo = 2;
constexpr std::uint32_t kBindingNormal = 3;
constexpr std::uint32_t kBindingRoughness = 4;
constexpr std::uint32_t kBindingMetallic = 5;
constexpr std::uint32_t kBindingAo = 6;
constexpr std::uint32_t kBindingSkybox = 7;
constexpr std::uint32_t kBindingTriangleBuffer = 8;
constexpr std::uint32_t kBindingBvhBuffer = 9;
constexpr std::uint32_t kBindingTriangleBufferGlass = 8;
constexpr std::uint32_t kBindingTriangleBufferGround = 9;
constexpr std::uint32_t kBindingUniform = 10;
constexpr std::uint32_t kBindingSkyboxBackground = 11;

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

struct CpuTriangle
{
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
};

struct GpuBvhNode
{
    glm::vec4 min;
    glm::vec4 max;
    glm::ivec4 meta;
};

static_assert(sizeof(GpuBvhNode) == 48);

constexpr std::size_t kFloatsPerVertex = 12;
constexpr std::size_t kFloatsPerTriangle = kFloatsPerVertex * 3;

CpuTriangle ReadCpuTriangle(const float* data, std::size_t tri_index)
{
    const std::size_t base = tri_index * kFloatsPerTriangle;
    CpuTriangle tri{};
    tri.v0 = glm::vec3(data[base + 0], data[base + 1], data[base + 2]);
    tri.v1 = glm::vec3(
        data[base + kFloatsPerVertex + 0],
        data[base + kFloatsPerVertex + 1],
        data[base + kFloatsPerVertex + 2]);
    tri.v2 = glm::vec3(
        data[base + kFloatsPerVertex * 2 + 0],
        data[base + kFloatsPerVertex * 2 + 1],
        data[base + kFloatsPerVertex * 2 + 2]);
    return tri;
}

std::vector<CpuTriangle> ReadTriangleBuffer(
    const frame::vulkan::Buffer& buffer)
{
    const auto& bytes = buffer.GetRawData();
    if (bytes.empty())
    {
        return {};
    }
    const std::size_t float_count = bytes.size() / sizeof(float);
    const std::size_t tri_count = float_count / kFloatsPerTriangle;
    std::vector<CpuTriangle> tris;
    tris.reserve(tri_count);
    const float* data = reinterpret_cast<const float*>(bytes.data());
    for (std::size_t i = 0; i < tri_count; ++i)
    {
        tris.push_back(ReadCpuTriangle(data, i));
    }
    return tris;
}

bool RayTriangleIntersect(
    const glm::vec3& ray_origin,
    const glm::vec3& ray_direction,
    const CpuTriangle& triangle,
    float& out_t)
{
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = triangle.v1 - triangle.v0;
    glm::vec3 edge2 = triangle.v2 - triangle.v0;
    glm::vec3 h = glm::cross(ray_direction, edge2);
    float a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;
    float f = 1.0f / a;
    glm::vec3 s = ray_origin - triangle.v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray_direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t = f * glm::dot(edge2, q);
    if (t > EPSILON)
    {
        out_t = t;
        return true;
    }
    return false;
}

struct CpuHit
{
    bool hit = false;
    float t = 0.0f;
    int material_id = -1;
};

CpuHit TraceCpuScene(
    const glm::vec3& ray_origin,
    const glm::vec3& ray_dir,
    const std::vector<CpuTriangle>& glass,
    const std::vector<CpuTriangle>& ground)
{
    CpuHit hit{};
    float best_t = std::numeric_limits<float>::max();
    for (const auto& tri : glass)
    {
        float t = 0.0f;
        if (RayTriangleIntersect(ray_origin, ray_dir, tri, t) && t < best_t)
        {
            best_t = t;
            hit.hit = true;
            hit.t = t;
            hit.material_id = 0;
        }
    }
    for (const auto& tri : ground)
    {
        float t = 0.0f;
        if (RayTriangleIntersect(ray_origin, ray_dir, tri, t) && t < best_t)
        {
            best_t = t;
            hit.hit = true;
            hit.t = t;
            hit.material_id = 1;
        }
    }
    return hit;
}

void ComputeRay(
    const UniformBlock& ubo,
    int x,
    int y,
    int width,
    int height,
    glm::vec3& out_origin,
    glm::vec3& out_dir)
{
    glm::vec2 uv =
        (glm::vec2(static_cast<float>(x), static_cast<float>(y)) +
         glm::vec2(0.5f)) /
        glm::vec2(static_cast<float>(width), static_cast<float>(height));
    uv.y = 1.0f - uv.y;
    glm::vec2 ndc = uv * 2.0f - 1.0f;
    glm::vec4 clip_pos(ndc, -1.0f, 1.0f);
    glm::vec4 view_pos = ubo.projection_inv * clip_pos;
    view_pos = glm::vec4(view_pos.x, view_pos.y, -1.0f, 0.0f);
    glm::vec3 ray_dir_world =
        glm::normalize(glm::vec3(ubo.view_inv * view_pos));

    glm::mat4 inv_model4 = ubo.model_inv;
    glm::mat3 model_inv3 = glm::mat3(inv_model4);
    float inv_det = std::abs(glm::determinant(model_inv3));
    if (inv_det < 1e-8f)
    {
        inv_model4 = glm::inverse(ubo.model);
        model_inv3 = glm::mat3(inv_model4);
    }
    out_origin = glm::vec3(
        inv_model4 *
        glm::vec4(
            ubo.camera_position.x,
            ubo.camera_position.y,
            ubo.camera_position.z,
            1.0f));
    out_dir = glm::normalize(model_inv3 * ray_dir_world);
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
    std::vector<float> sky_rgba(6 * 4, 1.0f); // 6 faces, 1 texel each.

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
    std::array<glm::vec4, kTriangleVec4Count> tri{};
    tri[0] = glm::vec4(-0.5f, -0.5f, 0.0f, 0.0f);
    tri[1] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    tri[2] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    tri[3] = glm::vec4(0.5f, -0.5f, 0.0f, 0.0f);
    tri[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    tri[5] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    tri[6] = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
    tri[7] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    tri[8] = glm::vec4(0.5f, 1.0f, 0.0f, 0.0f);

    vk::UniqueBuffer tri_buffer;
    auto tri_memory = MakeBufferWithData(
        tri.data(),
        sizeof(glm::vec4) * tri.size(),
        tri_buffer,
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
    ubo.env_map_model = glm::mat4(1.0f);
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
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {kBindingOutputImage,
         vk::DescriptorType::eStorageImage,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingOutputSampler,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAlbedo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingNormal,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingRoughness,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingMetallic,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkybox,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkyboxBackground,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkyboxBackground,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkyboxBackground,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkyboxBackground,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkyboxBackground,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBuffer,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingUniform,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
    };
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
    vk::UniqueDescriptorSetLayout set_layout =
        device_->createDescriptorSetLayoutUnique(layout_info);

    std::vector<vk::DescriptorPoolSize> pool_sizes = {
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eCombinedImageSampler, 8},
        {vk::DescriptorType::eStorageBuffer, 1},
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
        kBindingOutputImage,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    // Output sample binding uses output image sampler for completeness.
    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputSampler,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<std::pair<std::uint32_t, ImageResource*>, 7> textures = {
        std::pair{kBindingAlbedo, &albedo_tex},
        std::pair{kBindingNormal, &normal_tex},
        std::pair{kBindingRoughness, &roughness_tex},
        std::pair{kBindingMetallic, &metallic_tex},
        std::pair{kBindingAo, &ao_tex},
        std::pair{kBindingSkybox, &skybox_tex},
        std::pair{kBindingSkyboxBackground, &skybox_tex},
    };
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (const auto& entry : textures)
    {
        texture_infos.emplace_back(
            *entry.second->sampler,
            *entry.second->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            entry.first,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo tri_info(
        *tri_buffer, 0, sizeof(glm::vec4) * tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBuffer,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &tri_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        kBindingUniform,
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

TEST_F(VulkanRayTracingComputeTest, BvhDispatchMaskShowsHit)
{
    const auto shader_path = frame::file::FindFile(
        "asset/shader/vulkan/raytracing_bvh.comp");
    ASSERT_FALSE(shader_path.empty());
    vk::ShaderModule compute_module =
        CompileShader(shader_path, *device_);

    std::vector<float> black_rgba = {0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> normal_rgba = {0.5f, 0.5f, 1.0f, 1.0f};
    std::vector<float> sky_rgba(6 * 4, 1.0f); // 6 faces, 1 texel each.

    ImageResource albedo_tex = CreateTexture2D(black_rgba);
    ImageResource normal_tex = CreateTexture2D(normal_rgba);
    ImageResource roughness_tex = CreateTexture2D(black_rgba);
    ImageResource metallic_tex = CreateTexture2D(black_rgba);
    ImageResource ao_tex = CreateTexture2D(black_rgba);
    ImageResource skybox_tex = CreateCubemap(sky_rgba);

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

    auto make_triangle = [](float z) {
        std::array<glm::vec4, kTriangleVec4Count> tri{};
        tri[0] = glm::vec4(-0.5f, -0.5f, z, 0.0f);
        tri[1] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[2] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        tri[3] = glm::vec4(0.5f, -0.5f, z, 0.0f);
        tri[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[5] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        tri[6] = glm::vec4(0.0f, 0.5f, z, 0.0f);
        tri[7] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[8] = glm::vec4(0.5f, 1.0f, 0.0f, 0.0f);
        return tri;
    };

    std::array<glm::vec4, kTriangleVec4Count> tri = make_triangle(0.0f);
    GpuBvhNode node{};
    node.min = glm::vec4(-0.5f, -0.5f, 0.0f, 0.0f);
    node.max = glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
    node.meta = glm::ivec4(-1, -1, 0, 1);

    vk::UniqueBuffer tri_buffer;
    auto tri_memory = MakeBufferWithData(
        tri.data(),
        sizeof(glm::vec4) * tri.size(),
        tri_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);
    vk::UniqueBuffer bvh_buffer;
    auto bvh_memory = MakeBufferWithData(
        &node,
        sizeof(GpuBvhNode),
        bvh_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);

    UniformBlock ubo{};
    const glm::vec3 eye(0.0f, 0.0f, 1.5f);
    ubo.projection = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.projection_inv = glm::mat4(1.0f);
    ubo.view_inv = glm::mat4(1.0f);
    ubo.model = glm::mat4(1.0f);
    ubo.model_inv = glm::mat4(1.0f);
    ubo.env_map_model = glm::mat4(1.0f);
    ubo.camera_position = glm::vec4(eye, 1.0f);
    ubo.light_dir = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    ubo.light_color = glm::vec4(8.0f);
    ubo.time_s = glm::vec4(0.0f, 0.0f, 0.0f, 6.0f);
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

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {kBindingOutputImage,
         vk::DescriptorType::eStorageImage,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingOutputSampler,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAlbedo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingNormal,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingRoughness,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingMetallic,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkybox,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBuffer,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingBvhBuffer,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingUniform,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
    };
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
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

    std::vector<vk::WriteDescriptorSet> writes;
    vk::DescriptorImageInfo storage_image_info(
        nullptr, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputImage,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputSampler,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<std::pair<std::uint32_t, ImageResource*>, 7> textures = {
        std::pair{kBindingAlbedo, &albedo_tex},
        std::pair{kBindingNormal, &normal_tex},
        std::pair{kBindingRoughness, &roughness_tex},
        std::pair{kBindingMetallic, &metallic_tex},
        std::pair{kBindingAo, &ao_tex},
        std::pair{kBindingSkybox, &skybox_tex},
        std::pair{kBindingSkyboxBackground, &skybox_tex},
    };
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (const auto& entry : textures)
    {
        texture_infos.emplace_back(
            *entry.second->sampler,
            *entry.second->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            entry.first,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo tri_info(
        *tri_buffer, 0, sizeof(glm::vec4) * tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBuffer,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &tri_info);
    vk::DescriptorBufferInfo bvh_info(
        *bvh_buffer, 0, sizeof(GpuBvhNode));
    writes.emplace_back(
        descriptor_set,
        kBindingBvhBuffer,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &bvh_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        kBindingUniform,
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

    auto cmd = BeginCommands();
    cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipeline_layout,
        0,
        descriptor_set,
        {});
    cmd->dispatch(1, 1, 1);

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

    EXPECT_GT(rg.x, 0.9f);
    EXPECT_LT(rg.y, 0.1f);
    EXPECT_LT(ba.x, 0.1f);
    EXPECT_GE(ba.y, 0.9f);
    device_->destroyShaderModule(compute_module);
}

TEST_F(VulkanRayTracingComputeTest, DualDispatchMaskShowsGlassHit)
{
    const auto shader_path = frame::file::FindFile(
        "asset/shader/vulkan/raytracing_dual.comp");
    ASSERT_FALSE(shader_path.empty());
    vk::ShaderModule compute_module =
        CompileShader(shader_path, *device_);

    std::vector<float> black_rgba = {0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> normal_rgba = {0.5f, 0.5f, 1.0f, 1.0f};
    std::vector<float> sky_rgba(6 * 4, 1.0f); // 6 faces, 1 texel each.

    ImageResource albedo_tex = CreateTexture2D(black_rgba);
    ImageResource normal_tex = CreateTexture2D(normal_rgba);
    ImageResource roughness_tex = CreateTexture2D(black_rgba);
    ImageResource metallic_tex = CreateTexture2D(black_rgba);
    ImageResource ao_tex = CreateTexture2D(black_rgba);
    ImageResource skybox_tex = CreateCubemap(sky_rgba);

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

    auto make_triangle = [](float z) {
        std::array<glm::vec4, kTriangleVec4Count> tri{};
        tri[0] = glm::vec4(-0.5f, -0.5f, z, 0.0f);
        tri[1] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[2] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        tri[3] = glm::vec4(0.5f, -0.5f, z, 0.0f);
        tri[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[5] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        tri[6] = glm::vec4(0.0f, 0.5f, z, 0.0f);
        tri[7] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[8] = glm::vec4(0.5f, 1.0f, 0.0f, 0.0f);
        return tri;
    };

    std::array<glm::vec4, kTriangleVec4Count> glass_tri = make_triangle(0.0f);
    std::array<glm::vec4, kTriangleVec4Count> ground_tri = make_triangle(-2.0f);

    vk::UniqueBuffer glass_buffer;
    auto glass_memory = MakeBufferWithData(
        glass_tri.data(),
        sizeof(glm::vec4) * glass_tri.size(),
        glass_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);
    vk::UniqueBuffer ground_buffer;
    auto ground_memory = MakeBufferWithData(
        ground_tri.data(),
        sizeof(glm::vec4) * ground_tri.size(),
        ground_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);

    UniformBlock ubo{};
    const glm::vec3 eye(0.0f, 0.0f, 1.5f);
    ubo.projection = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.projection_inv = glm::mat4(1.0f);
    ubo.view_inv = glm::mat4(1.0f);
    ubo.model = glm::mat4(1.0f);
    ubo.model_inv = glm::mat4(1.0f);
    ubo.env_map_model = glm::mat4(1.0f);
    ubo.camera_position = glm::vec4(eye, 1.0f);
    ubo.light_dir = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    ubo.light_color = glm::vec4(8.0f);
    ubo.time_s = glm::vec4(0.0f, 0.0f, 0.0f, 6.0f);
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

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {kBindingOutputImage,
         vk::DescriptorType::eStorageImage,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingOutputSampler,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAlbedo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingNormal,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingRoughness,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingMetallic,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkybox,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGlass,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGround,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingUniform,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
    };
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
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

    std::vector<vk::WriteDescriptorSet> writes;
    vk::DescriptorImageInfo storage_image_info(
        nullptr, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputImage,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputSampler,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<std::pair<std::uint32_t, ImageResource*>, 7> textures = {
        std::pair{kBindingAlbedo, &albedo_tex},
        std::pair{kBindingNormal, &normal_tex},
        std::pair{kBindingRoughness, &roughness_tex},
        std::pair{kBindingMetallic, &metallic_tex},
        std::pair{kBindingAo, &ao_tex},
        std::pair{kBindingSkybox, &skybox_tex},
        std::pair{kBindingSkyboxBackground, &skybox_tex},
    };
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (const auto& entry : textures)
    {
        texture_infos.emplace_back(
            *entry.second->sampler,
            *entry.second->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            entry.first,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo glass_info(
        *glass_buffer, 0, sizeof(glm::vec4) * glass_tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGlass,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &glass_info);
    vk::DescriptorBufferInfo ground_info(
        *ground_buffer, 0, sizeof(glm::vec4) * ground_tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGround,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &ground_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        kBindingUniform,
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

    auto cmd = BeginCommands();
    cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipeline_layout,
        0,
        descriptor_set,
        {});
    cmd->dispatch(1, 1, 1);

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

    EXPECT_GT(rg.x, 0.9f);
    EXPECT_LT(rg.y, 0.1f);
    EXPECT_LT(ba.x, 0.1f);
    EXPECT_GE(ba.y, 0.9f);
    device_->destroyShaderModule(compute_module);
}

TEST_F(VulkanRayTracingComputeTest, DualDispatchMaskShowsGroundHit)
{
    const auto shader_path = frame::file::FindFile(
        "asset/shader/vulkan/raytracing_dual.comp");
    ASSERT_FALSE(shader_path.empty());
    vk::ShaderModule compute_module =
        CompileShader(shader_path, *device_);

    std::vector<float> black_rgba = {0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> normal_rgba = {0.5f, 0.5f, 1.0f, 1.0f};
    std::vector<float> sky_rgba(6 * 4, 1.0f);

    ImageResource albedo_tex = CreateTexture2D(black_rgba);
    ImageResource normal_tex = CreateTexture2D(normal_rgba);
    ImageResource roughness_tex = CreateTexture2D(black_rgba);
    ImageResource metallic_tex = CreateTexture2D(black_rgba);
    ImageResource ao_tex = CreateTexture2D(black_rgba);
    ImageResource skybox_tex = CreateCubemap(sky_rgba);

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

    auto make_triangle = [](float z) {
        std::array<glm::vec4, kTriangleVec4Count> tri{};
        tri[0] = glm::vec4(-0.5f, -0.5f, z, 0.0f);
        tri[1] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[2] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        tri[3] = glm::vec4(0.5f, -0.5f, z, 0.0f);
        tri[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[5] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        tri[6] = glm::vec4(0.0f, 0.5f, z, 0.0f);
        tri[7] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        tri[8] = glm::vec4(0.5f, 1.0f, 0.0f, 0.0f);
        return tri;
    };

    std::array<glm::vec4, kTriangleVec4Count> glass_tri = make_triangle(-2.0f);
    std::array<glm::vec4, kTriangleVec4Count> ground_tri = make_triangle(0.0f);

    vk::UniqueBuffer glass_buffer;
    auto glass_memory = MakeBufferWithData(
        glass_tri.data(),
        sizeof(glm::vec4) * glass_tri.size(),
        glass_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);
    vk::UniqueBuffer ground_buffer;
    auto ground_memory = MakeBufferWithData(
        ground_tri.data(),
        sizeof(glm::vec4) * ground_tri.size(),
        ground_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);

    UniformBlock ubo{};
    const glm::vec3 eye(0.0f, 0.0f, 1.5f);
    ubo.projection = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.projection_inv = glm::mat4(1.0f);
    ubo.view_inv = glm::mat4(1.0f);
    ubo.model = glm::mat4(1.0f);
    ubo.model_inv = glm::mat4(1.0f);
    ubo.env_map_model = glm::mat4(1.0f);
    ubo.camera_position = glm::vec4(eye, 1.0f);
    ubo.light_dir = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    ubo.light_color = glm::vec4(8.0f);
    ubo.time_s = glm::vec4(0.0f, 0.0f, 0.0f, 6.0f);
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

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {kBindingOutputImage,
         vk::DescriptorType::eStorageImage,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingOutputSampler,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAlbedo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingNormal,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingRoughness,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingMetallic,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkybox,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGlass,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGround,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingUniform,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
    };
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
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

    std::vector<vk::WriteDescriptorSet> writes;
    vk::DescriptorImageInfo storage_image_info(
        nullptr, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputImage,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputSampler,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<std::pair<std::uint32_t, ImageResource*>, 7> textures = {
        std::pair{kBindingAlbedo, &albedo_tex},
        std::pair{kBindingNormal, &normal_tex},
        std::pair{kBindingRoughness, &roughness_tex},
        std::pair{kBindingMetallic, &metallic_tex},
        std::pair{kBindingAo, &ao_tex},
        std::pair{kBindingSkybox, &skybox_tex},
        std::pair{kBindingSkyboxBackground, &skybox_tex},
    };
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (const auto& entry : textures)
    {
        texture_infos.emplace_back(
            *entry.second->sampler,
            *entry.second->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            entry.first,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo glass_info(
        *glass_buffer, 0, sizeof(glm::vec4) * glass_tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGlass,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &glass_info);
    vk::DescriptorBufferInfo ground_info(
        *ground_buffer, 0, sizeof(glm::vec4) * ground_tri.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGround,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &ground_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        kBindingUniform,
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

    auto cmd = BeginCommands();
    cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipeline_layout,
        0,
        descriptor_set,
        {});
    cmd->dispatch(1, 1, 1);

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

    EXPECT_LT(rg.x, 0.1f);
    EXPECT_GT(rg.y, 0.9f);
    EXPECT_LT(ba.x, 0.1f);
    EXPECT_GE(ba.y, 0.9f);
    device_->destroyShaderModule(compute_module);
}

TEST_F(VulkanRayTracingComputeTest, SceneHitMaskMatchesCpuIntersections)
{
    constexpr std::uint32_t kWidth = 32;
    constexpr std::uint32_t kHeight = 32;

    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_path = frame::file::FindFile("asset/json/raytracing.json");
    ASSERT_FALSE(level_path.empty());
    auto level_proto = frame::json::LoadLevelProto(level_path);
    auto level_data = frame::json::ParseLevelData(
        glm::uvec2(kWidth, kHeight), level_proto, asset_root);
    auto built = frame::vulkan::BuildLevel(glm::uvec2(kWidth, kHeight), level_data);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    frame::EntityId glass_id = frame::NullId;
    frame::EntityId ground_id = frame::NullId;
    for (const auto& name : material.GetBufferNames())
    {
        const auto inner = material.GetInnerBufferName(name);
        if (inner == "TriangleBufferGlass")
        {
            glass_id = level.GetIdFromName(name);
        }
        else if (inner == "TriangleBufferGround")
        {
            ground_id = level.GetIdFromName(name);
        }
    }
    ASSERT_NE(glass_id, frame::NullId);
    ASSERT_NE(ground_id, frame::NullId);

    auto* glass_cpu =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(glass_id));
    auto* ground_cpu =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(ground_id));
    ASSERT_NE(glass_cpu, nullptr);
    ASSERT_NE(ground_cpu, nullptr);

    const auto glass_tris = ReadTriangleBuffer(*glass_cpu);
    const auto ground_tris = ReadTriangleBuffer(*ground_cpu);
    ASSERT_FALSE(glass_tris.empty());
    ASSERT_FALSE(ground_tris.empty());

    const auto shader_path = frame::file::FindFile(
        "asset/shader/vulkan/raytracing_dual.comp");
    ASSERT_FALSE(shader_path.empty());
    vk::ShaderModule compute_module =
        CompileShader(shader_path, *device_);

    std::vector<float> black_rgba = {0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> normal_rgba = {0.5f, 0.5f, 1.0f, 1.0f};
    std::vector<float> sky_rgba(6 * 4, 1.0f);

    ImageResource albedo_tex = CreateTexture2D(black_rgba);
    ImageResource normal_tex = CreateTexture2D(normal_rgba);
    ImageResource roughness_tex = CreateTexture2D(black_rgba);
    ImageResource metallic_tex = CreateTexture2D(black_rgba);
    ImageResource ao_tex = CreateTexture2D(black_rgba);
    ImageResource skybox_tex = CreateCubemap(sky_rgba);

    vk::ImageCreateInfo out_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        vk::Format::eR16G16B16A16Sfloat,
        {kWidth, kHeight, 1},
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

    const auto& glass_bytes = glass_cpu->GetRawData();
    const auto& ground_bytes = ground_cpu->GetRawData();
    vk::UniqueBuffer glass_buffer;
    auto glass_memory = MakeBufferWithData(
        glass_bytes.data(),
        glass_bytes.size(),
        glass_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);
    vk::UniqueBuffer ground_buffer;
    auto ground_memory = MakeBufferWithData(
        ground_bytes.data(),
        ground_bytes.size(),
        ground_buffer,
        vk::BufferUsageFlagBits::eStorageBuffer);

    auto scene_state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {kWidth, kHeight},
        0.0f,
        material_id,
        false);
    UniformBlock ubo = frame::vulkan::MakeUniformBlock(scene_state, 0.0f);
    ubo.time_s.w = 6.0f;
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

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {kBindingOutputImage,
         vk::DescriptorType::eStorageImage,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingOutputSampler,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAlbedo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingNormal,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingRoughness,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingMetallic,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingAo,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingSkybox,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGlass,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingTriangleBufferGround,
         vk::DescriptorType::eStorageBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
        {kBindingUniform,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eCompute},
    };
    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
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

    std::vector<vk::WriteDescriptorSet> writes;
    vk::DescriptorImageInfo storage_image_info(
        nullptr, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputImage,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &storage_image_info);

    vk::DescriptorImageInfo output_sample(
        *out_sampler, *out_view, vk::ImageLayout::eGeneral);
    writes.emplace_back(
        descriptor_set,
        kBindingOutputSampler,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &output_sample);

    const std::array<std::pair<std::uint32_t, ImageResource*>, 7> textures = {
        std::pair{kBindingAlbedo, &albedo_tex},
        std::pair{kBindingNormal, &normal_tex},
        std::pair{kBindingRoughness, &roughness_tex},
        std::pair{kBindingMetallic, &metallic_tex},
        std::pair{kBindingAo, &ao_tex},
        std::pair{kBindingSkybox, &skybox_tex},
        std::pair{kBindingSkyboxBackground, &skybox_tex},
    };
    std::vector<vk::DescriptorImageInfo> texture_infos;
    texture_infos.reserve(textures.size());
    for (const auto& entry : textures)
    {
        texture_infos.emplace_back(
            *entry.second->sampler,
            *entry.second->view,
            vk::ImageLayout::eShaderReadOnlyOptimal);
        writes.emplace_back(
            descriptor_set,
            entry.first,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos.back());
    }

    vk::DescriptorBufferInfo glass_info(
        *glass_buffer, 0, glass_bytes.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGlass,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &glass_info);
    vk::DescriptorBufferInfo ground_info(
        *ground_buffer, 0, ground_bytes.size());
    writes.emplace_back(
        descriptor_set,
        kBindingTriangleBufferGround,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &ground_info);
    vk::DescriptorBufferInfo uniform_info(
        *uniform_buffer, 0, sizeof(UniformBlock));
    writes.emplace_back(
        descriptor_set,
        kBindingUniform,
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

    auto cmd = BeginCommands();
    cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipeline_layout,
        0,
        descriptor_set,
        {});
    const std::uint32_t group_x = (kWidth + 7) / 8;
    const std::uint32_t group_y = (kHeight + 7) / 8;
    cmd->dispatch(group_x, group_y, 1);

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

    const vk::DeviceSize copy_size =
        static_cast<vk::DeviceSize>(kWidth) *
        static_cast<vk::DeviceSize>(kHeight) *
        sizeof(std::uint16_t) * 4;
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
        {kWidth, kHeight, 1});
    cmd->copyImageToBuffer(
        *out_image,
        vk::ImageLayout::eTransferSrcOptimal,
        *readback,
        region);
    EndCommands(cmd);

    const auto* raw16 = static_cast<const std::uint16_t*>(
        device_->mapMemory(*readback_memory, 0, copy_size));
    auto decode_pixel = [&](std::size_t index) {
        const std::size_t base = index * 4;
        glm::vec2 rg = glm::unpackHalf2x16(
            (static_cast<std::uint32_t>(raw16[base + 1]) << 16) |
            raw16[base + 0]);
        glm::vec2 ba = glm::unpackHalf2x16(
            (static_cast<std::uint32_t>(raw16[base + 3]) << 16) |
            raw16[base + 2]);
        return glm::vec4(rg.x, rg.y, ba.x, ba.y);
    };

    int cpu_glass_hits = 0;
    int cpu_ground_hits = 0;
    int gpu_glass_hits = 0;
    int gpu_ground_hits = 0;
    int mismatches = 0;

    for (std::uint32_t y = 0; y < kHeight; ++y)
    {
        for (std::uint32_t x = 0; x < kWidth; ++x)
        {
            glm::vec3 ray_origin;
            glm::vec3 ray_dir;
            ComputeRay(
                ubo,
                static_cast<int>(x),
                static_cast<int>(y),
                static_cast<int>(kWidth),
                static_cast<int>(kHeight),
                ray_origin,
                ray_dir);
            CpuHit cpu_hit = TraceCpuScene(
                ray_origin, ray_dir, glass_tris, ground_tris);
            int expected = cpu_hit.hit ? cpu_hit.material_id : -1;

            const std::size_t pixel_index =
                static_cast<std::size_t>(y) * kWidth + x;
            glm::vec4 color = decode_pixel(pixel_index);
            int actual = -1;
            if (color.g > 0.5f)
            {
                actual = 1;
            }
            else if (color.r > 0.5f)
            {
                actual = 0;
            }

            if (expected != actual)
            {
                ++mismatches;
            }
            if (expected == 0)
            {
                ++cpu_glass_hits;
            }
            else if (expected == 1)
            {
                ++cpu_ground_hits;
            }
            if (actual == 0)
            {
                ++gpu_glass_hits;
            }
            else if (actual == 1)
            {
                ++gpu_ground_hits;
            }
        }
    }

    device_->unmapMemory(*readback_memory);

    EXPECT_GT(cpu_glass_hits, 0);
    EXPECT_GT(cpu_ground_hits, 0);
    EXPECT_GT(gpu_glass_hits, 0);
    EXPECT_GT(gpu_ground_hits, 0);
    EXPECT_EQ(mismatches, 0);

    device_->destroyShaderModule(compute_module);
}

} // namespace test
