#include "frame/vulkan/command_queue.h"

#include <functional>
#include <stdexcept>

namespace frame::vulkan
{

vk::CommandBuffer CommandQueue::BeginOneTime() const
{
    vk::CommandBufferAllocateInfo alloc_info(
        pool_,
        vk::CommandBufferLevel::ePrimary,
        1);
    auto command_buffers = device_.allocateCommandBuffers(alloc_info);
    auto command_buffer = command_buffers.front();
    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);
    return command_buffer;
}

void CommandQueue::EndOneTime(vk::CommandBuffer command_buffer) const
{
    command_buffer.end();
    vk::SubmitInfo submit_info(
        0, nullptr, nullptr,
        1, &command_buffer,
        0, nullptr);
    queue_.submit(submit_info);
    queue_.waitIdle();
    device_.freeCommandBuffers(pool_, command_buffer);
}

void CommandQueue::CopyBuffer(
    vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) const
{
    auto command_buffer = BeginOneTime();
    vk::BufferCopy copy_region(0, 0, size);
    command_buffer.copyBuffer(src, dst, copy_region);
    EndOneTime(command_buffer);
}

void CommandQueue::CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t layer_count,
    std::size_t layer_stride) const
{
    auto command_buffer = BeginOneTime();
    std::vector<vk::BufferImageCopy> copies;
    copies.reserve(layer_count);
    const std::size_t stride =
        (layer_stride == 0)
            ? static_cast<std::size_t>(width) * height
            : layer_stride;
    for (std::uint32_t layer = 0; layer < layer_count; ++layer)
    {
        copies.emplace_back(
            stride * layer,
            0,
            0,
            vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor, 0, layer, 1},
            vk::Offset3D{0, 0, 0},
            vk::Extent3D{width, height, 1});
    }

    command_buffer.copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        copies);
    EndOneTime(command_buffer);
}

void CommandQueue::TransitionImageLayout(
    vk::Image image,
    vk::Format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    std::uint32_t layer_count) const
{
    auto command_buffer = BeginOneTime();

    vk::ImageMemoryBarrier barrier(
        {},
        {},
        old_layout,
        new_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});

    vk::PipelineStageFlags src_stage;
    vk::PipelineStageFlags dst_stage;

    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (old_layout == vk::ImageLayout::eUndefined &&
             new_layout == vk::ImageLayout::eGeneral)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eComputeShader;
    }
    else if (old_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
             new_layout == vk::ImageLayout::eGeneral)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
        src_stage = vk::PipelineStageFlagBits::eFragmentShader |
            vk::PipelineStageFlagBits::eComputeShader;
        dst_stage = vk::PipelineStageFlagBits::eComputeShader;
    }
    else if (old_layout == vk::ImageLayout::eGeneral &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eComputeShader;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader |
            vk::PipelineStageFlagBits::eComputeShader;
    }
    else if (old_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
             new_layout == vk::ImageLayout::eTransferSrcOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        src_stage = vk::PipelineStageFlagBits::eFragmentShader |
            vk::PipelineStageFlagBits::eComputeShader;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eGeneral &&
             new_layout == vk::ImageLayout::eTransferSrcOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        src_stage = vk::PipelineStageFlagBits::eComputeShader;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferSrcOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader |
            vk::PipelineStageFlagBits::eComputeShader;
    }
    else
    {
        throw std::runtime_error("Unsupported Vulkan image layout transition.");
    }

    command_buffer.pipelineBarrier(
        src_stage,
        dst_stage,
        {},
        nullptr,
        nullptr,
        barrier);
    EndOneTime(command_buffer);
}

void CommandQueue::SubmitOneTime(
    const std::function<void(vk::CommandBuffer)>& recorder) const
{
    auto command_buffer = BeginOneTime();
    recorder(command_buffer);
    EndOneTime(command_buffer);
}

} // namespace frame::vulkan
