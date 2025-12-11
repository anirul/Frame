#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#ifndef VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1
#endif

#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>
#include <cstring>
#include <vector>
#include <optional>

#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"

namespace test
{

class VulkanCommandQueueTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto vk_get_instance_proc_addr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr);
        ASSERT_NE(vk_get_instance_proc_addr, nullptr);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

        vk::ApplicationInfo app_info(
            "CommandQueueTest", 1, "Frame", 1, VK_API_VERSION_1_1);
        vk::InstanceCreateInfo instance_info({}, &app_info);
        instance_ = vk::createInstanceUnique(instance_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
        auto physical_devices = instance_->enumeratePhysicalDevices();
        ASSERT_FALSE(physical_devices.empty());
        physical_device_ = physical_devices.front();

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
        device_->waitIdle();
    }

    std::unique_ptr<frame::vulkan::GpuMemoryManager> memory_manager_;
    std::unique_ptr<frame::vulkan::CommandQueue> command_queue_;
    vk::UniqueInstance instance_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vk::Queue queue_;
    vk::UniqueCommandPool command_pool_;
    std::uint32_t graphics_family_index_ = 0;
};

TEST_F(VulkanCommandQueueTest, CopiesBufferContents)
{
    constexpr vk::DeviceSize kSize = 64;
    vk::UniqueDeviceMemory src_memory;
    vk::UniqueDeviceMemory dst_memory;
    auto src_buffer = memory_manager_->CreateBuffer(
        kSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        src_memory);
    auto dst_buffer = memory_manager_->CreateBuffer(
        kSize,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        dst_memory);

    void* mapped_src = device_->mapMemory(*src_memory, 0, kSize);
    std::vector<std::uint8_t> pattern(kSize);
    for (std::size_t i = 0; i < kSize; ++i)
    {
        pattern[i] = static_cast<std::uint8_t>(i);
    }
    std::memcpy(mapped_src, pattern.data(), kSize);
    device_->unmapMemory(*src_memory);

    command_queue_->CopyBuffer(*src_buffer, *dst_buffer, kSize);

    void* mapped_dst = device_->mapMemory(*dst_memory, 0, kSize);
    std::vector<std::uint8_t> result(kSize);
    std::memcpy(result.data(), mapped_dst, kSize);
    device_->unmapMemory(*dst_memory);

    EXPECT_EQ(pattern, result);
}

TEST_F(VulkanCommandQueueTest, TransitionsAndCopiesToImage)
{
    constexpr std::uint32_t kWidth = 2;
    constexpr std::uint32_t kHeight = 2;
    constexpr std::uint32_t kPixelCount = kWidth * kHeight;

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = memory_manager_->CreateBuffer(
        kPixelCount * sizeof(std::uint32_t),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);

    std::vector<std::uint32_t> pixels = {
        0xff0000ffu, 0xff00ff00u, 0xffff0000u, 0xffffffffu};
    void* mapped = device_->mapMemory(
        *staging_memory, 0, pixels.size() * sizeof(std::uint32_t));
    std::memcpy(mapped, pixels.data(), pixels.size() * sizeof(std::uint32_t));
    device_->unmapMemory(*staging_memory);

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Unorm,
        vk::Extent3D{kWidth, kHeight, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled);
    auto image = device_->createImageUnique(image_info);
    auto requirements = device_->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    auto image_memory = device_->allocateMemoryUnique(allocate_info);
    device_->bindImageMemory(*image, *image_memory, 0);

    command_queue_->TransitionImageLayout(
        *image,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);
    command_queue_->CopyBufferToImage(
        *staging_buffer, *image, kWidth, kHeight);
    command_queue_->TransitionImageLayout(
        *image,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    // If we reach here without throwing, the queue handled the commands.
    SUCCEED();
}

} // namespace test
