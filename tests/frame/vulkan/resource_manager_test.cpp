#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <optional>
#include <vector>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/vulkan_dispatch.h"

namespace test
{
namespace
{

class VulkanResourceFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto vk_get_instance_proc_addr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr);
        ASSERT_NE(vk_get_instance_proc_addr, nullptr);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

        vk::ApplicationInfo app_info(
            "VulkanResourceTest",
            1,
            "Frame",
            1,
            VK_API_VERSION_1_1);
        vk::InstanceCreateInfo instance_info({}, &app_info);
        try
        {
            instance_ = vk::createInstanceUnique(instance_info);
        }
        catch (const vk::SystemError& err)
        {
            GTEST_SKIP()
                << "Skipping Vulkan tests: unable to create instance ("
                << err.what() << ").";
            return;
        }
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

        auto physical_devices = instance_->enumeratePhysicalDevices();
        if (physical_devices.empty())
        {
            GTEST_SKIP()
                << "Skipping Vulkan tests: no physical devices found.";
            return;
        }
        physical_device_ = physical_devices.front();
        if (physical_device_.getProperties().deviceType ==
            vk::PhysicalDeviceType::eCpu)
        {
            GTEST_SKIP() << "Skipping on CPU Vulkan device.";
            return;
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
        vk::DeviceCreateInfo device_info({}, 1, &queue_info);
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

    vk::UniqueInstance instance_;
    vk::PhysicalDevice physical_device_;
    std::uint32_t graphics_family_index_ = 0;
    vk::UniqueDevice device_;
    vk::Queue queue_;
    vk::UniqueCommandPool command_pool_;
    std::unique_ptr<frame::vulkan::GpuMemoryManager> memory_manager_;
    std::unique_ptr<frame::vulkan::CommandQueue> command_queue_;
};

} // namespace

TEST_F(VulkanResourceFixture, MeshResourcesBuildFallbackQuad)
{
    frame::json::LevelData level_data{};
    frame::vulkan::MeshResources resources(
        *device_,
        *memory_manager_,
        *command_queue_,
        frame::Logger::GetInstance());

    resources.Build(level_data);

    ASSERT_FALSE(resources.Empty());
    const auto& mesh = resources.GetMeshes().front();
    EXPECT_EQ(mesh.index_count, 6u);
    EXPECT_TRUE(static_cast<bool>(mesh.vertex_buffer));
    EXPECT_TRUE(static_cast<bool>(mesh.index_buffer));
}

TEST_F(VulkanResourceFixture, MeshResourcesUploadCustomMesh)
{
    frame::json::LevelData level_data{};
    frame::json::StaticMeshInfo mesh{};
    mesh.name = "Triangle";
    mesh.positions = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    mesh.uvs = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    mesh.indices = {0, 1, 2};
    level_data.meshes.push_back(mesh);

    frame::vulkan::MeshResources resources(
        *device_,
        *memory_manager_,
        *command_queue_,
        frame::Logger::GetInstance());
    resources.Build(level_data);

    ASSERT_FALSE(resources.Empty());
    const auto& gpu_mesh = resources.GetMeshes().front();
    EXPECT_EQ(gpu_mesh.index_count, 3u);
    EXPECT_TRUE(static_cast<bool>(gpu_mesh.vertex_buffer));
    EXPECT_TRUE(static_cast<bool>(gpu_mesh.index_buffer));
}

TEST_F(VulkanResourceFixture, BufferResourcesMirrorCpuBuffer)
{
    frame::Level level;
    auto buffer = std::make_unique<frame::vulkan::Buffer>();
    std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f};
    buffer->Copy(values);
    buffer->SetName("TriangleBuffer");
    auto buffer_id = level.AddBuffer(std::move(buffer));

    frame::vulkan::BufferResourceManager manager(
        *device_,
        *memory_manager_,
        *command_queue_,
        frame::Logger::GetInstance());
    manager.BuildStorageBuffers(level, {buffer_id});

    const auto& gpu_buffers = manager.GetStorageBuffers();
    ASSERT_EQ(gpu_buffers.size(), 1u);
    EXPECT_EQ(
        gpu_buffers.front().size,
        values.size() * sizeof(float));
    EXPECT_TRUE(static_cast<bool>(gpu_buffers.front().buffer));
}

TEST_F(VulkanResourceFixture, BufferResourcesUploadDataVisible)
{
    frame::Level level;
    auto buffer = std::make_unique<frame::vulkan::Buffer>();
    std::vector<float> values = {0.25f, 1.5f, -2.0f, 4.25f};
    buffer->Copy(values);
    buffer->SetName("VisibilityBuffer");
    auto buffer_id = level.AddBuffer(std::move(buffer));

    frame::vulkan::BufferResourceManager manager(
        *device_,
        *memory_manager_,
        *command_queue_,
        frame::Logger::GetInstance());
    manager.BuildStorageBuffers(level, {buffer_id});

    const auto& gpu_buffers = manager.GetStorageBuffers();
    ASSERT_EQ(gpu_buffers.size(), 1u);
    ASSERT_TRUE(static_cast<bool>(gpu_buffers.front().buffer));
    ASSERT_EQ(
        gpu_buffers.front().size,
        values.size() * sizeof(float));

    vk::UniqueDeviceMemory readback_memory;
    auto readback = memory_manager_->CreateBuffer(
        gpu_buffers.front().size,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        readback_memory);

    command_queue_->CopyBuffer(
        *gpu_buffers.front().buffer,
        *readback,
        gpu_buffers.front().size);

    std::vector<float> out(values.size(), 0.0f);
    void* mapped = device_->mapMemory(
        *readback_memory, 0, gpu_buffers.front().size);
    ASSERT_NE(mapped, nullptr);
    std::memcpy(out.data(), mapped, gpu_buffers.front().size);
    device_->unmapMemory(*readback_memory);

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_FLOAT_EQ(out[i], values[i]);
    }
}

TEST_F(VulkanResourceFixture, BufferResourcesUpdateUniformData)
{
    frame::vulkan::BufferResourceManager manager(
        *device_,
        *memory_manager_,
        *command_queue_,
        frame::Logger::GetInstance());
    manager.BuildUniformBuffer(sizeof(frame::vulkan::UniformBlock));

    frame::vulkan::UniformBlock block{};
    block.camera_position = glm::vec4(1.0f, 2.0f, 3.0f, 0.0f);
    manager.UpdateUniform(&block, sizeof(block));

    const auto* uniform = manager.GetUniformBuffer();
    ASSERT_NE(uniform, nullptr);
    auto mapped = static_cast<const frame::vulkan::UniformBlock*>(
        device_->mapMemory(*uniform->memory, 0, uniform->size));
    ASSERT_NE(mapped, nullptr);
    EXPECT_FLOAT_EQ(mapped->camera_position.x, 1.0f);
    EXPECT_FLOAT_EQ(mapped->camera_position.y, 2.0f);
    EXPECT_FLOAT_EQ(mapped->camera_position.z, 3.0f);
    device_->unmapMemory(*uniform->memory);
}

} // namespace test
