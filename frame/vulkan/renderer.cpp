#include "frame/vulkan/renderer.h"

#include <cstdint>
#include <limits>

#include "frame/vulkan/device.h"
#include "frame/vulkan/command_resources.h"
#include "frame/vulkan/pipeline_resources.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/sync_resources.h"

namespace frame::vulkan
{

Renderer::Renderer(Device& device) : device_(device)
{
}

void Renderer::Display(double dt)
{
    if (device_.device_lost_)
    {
        return;
    }

    device_.elapsed_time_seconds_ += static_cast<float>(dt);

    if (device_.level_)
    {
        device_.level_->UpdateLights(
            static_cast<double>(device_.elapsed_time_seconds_));
        device_.UpdateRaytraceBuffers();
    }

    if (!device_.vk_unique_device_ || !device_.swapchain_resources_ ||
        !device_.swapchain_resources_->IsValid())
    {
        return;
    }
    if (!device_.command_resources_ ||
        device_.command_resources_->GetBuffers().empty())
    {
        return;
    }
    if (!device_.sync_resources_ || !device_.sync_resources_->IsCreated())
    {
        return;
    }
    const bool has_scene_pipeline =
        device_.pipeline_resources_ &&
        device_.pipeline_resources_->HasGraphicsPipeline() &&
        device_.pipeline_resources_->GetGraphicsPipelineLayout();
    const bool has_gui_render = static_cast<bool>(device_.gui_render_callback_);
    if (!has_scene_pipeline && !has_gui_render)
    {
        return;
    }

    if (device_.framebuffer_resized_)
    {
        device_.framebuffer_resized_ = false;
        device_.RecreateSwapchain();
        return;
    }

    const vk::Fence fence =
        device_.sync_resources_->GetInFlightFence(current_frame_);
    const VkFence fence_handle = static_cast<VkFence>(fence);
    const VkResult wait_result = vkWaitForFences(
        static_cast<VkDevice>(*device_.vk_unique_device_),
        1,
        &fence_handle,
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());
    if (wait_result != VK_SUCCESS)
    {
        device_.logger_->error(
            "vkWaitForFences failed: {}",
            vk::to_string(static_cast<vk::Result>(wait_result)));
        if (wait_result == VK_ERROR_DEVICE_LOST)
        {
            device_.device_lost_ = true;
        }
        return;
    }

    const auto& swapchain = device_.swapchain_resources_->GetSwapchain();
    auto acquire = device_.vk_unique_device_->acquireNextImageKHR(
        *swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        device_.sync_resources_->GetImageAvailable(current_frame_),
        nullptr);

    if (acquire.result == vk::Result::eErrorOutOfDateKHR)
    {
        device_.RecreateSwapchain();
        return;
    }
    if (acquire.result != vk::Result::eSuccess &&
        acquire.result != vk::Result::eSuboptimalKHR)
    {
        device_.logger_->error(
            "Failed to acquire swapchain image: {}",
            vk::to_string(acquire.result));
        if (acquire.result == vk::Result::eErrorDeviceLost)
        {
            device_.device_lost_ = true;
        }
        return;
    }

    const std::uint32_t image_index = acquire.value;
    const VkResult reset_result = vkResetFences(
        static_cast<VkDevice>(*device_.vk_unique_device_),
        1,
        &fence_handle);
    if (reset_result != VK_SUCCESS)
    {
        device_.logger_->error(
            "vkResetFences failed: {}",
            vk::to_string(static_cast<vk::Result>(reset_result)));
        if (reset_result == VK_ERROR_DEVICE_LOST)
        {
            device_.device_lost_ = true;
        }
        return;
    }

    vk::CommandBuffer command_buffer =
        device_.command_resources_->GetBuffer(current_frame_);
    command_buffer.reset();
    device_.RecordCommandBuffer(command_buffer, image_index);

    const vk::Semaphore wait_semaphores[] = {
        device_.sync_resources_->GetImageAvailable(current_frame_)};
    const vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::Semaphore signal_semaphores[] = {
        device_.sync_resources_->GetRenderFinished(image_index)};

    vk::SubmitInfo submit_info(
        1,
        wait_semaphores,
        wait_stages,
        1,
        &command_buffer,
        1,
        signal_semaphores);

    const VkSubmitInfo submit_info_c = submit_info;
    const VkResult submit_result = vkQueueSubmit(
        static_cast<VkQueue>(device_.graphics_queue_),
        1,
        &submit_info_c,
        fence);
    if (submit_result != VK_SUCCESS)
    {
        device_.logger_->error(
            "vkQueueSubmit failed: {}",
            vk::to_string(static_cast<vk::Result>(submit_result)));
        if (submit_result == VK_ERROR_DEVICE_LOST)
        {
            device_.device_lost_ = true;
        }
        return;
    }

    vk::PresentInfoKHR present_info(
        1,
        signal_semaphores,
        1,
        &swapchain.get(),
        &image_index);

    const vk::Result present_result =
        device_.present_queue_.presentKHR(present_info);
    if (present_result == vk::Result::eErrorOutOfDateKHR ||
        present_result == vk::Result::eSuboptimalKHR)
    {
        device_.RecreateSwapchain();
    }
    else if (present_result != vk::Result::eSuccess)
    {
        device_.logger_->error(
            "Failed to present swapchain image: {}",
            vk::to_string(present_result));
        if (present_result == vk::Result::eErrorDeviceLost)
        {
            device_.device_lost_ = true;
        }
        return;
    }

    current_frame_ = (current_frame_ + 1) % device_.kMaxFramesInFlight;
}

void Renderer::Reset()
{
    current_frame_ = 0;
}

} // namespace frame::vulkan
