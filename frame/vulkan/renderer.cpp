#include "frame/vulkan/renderer.h"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/gtc/matrix_inverse.hpp>

#include "frame/camera.h"
#include "frame/vulkan/device.h"
#include "frame/vulkan/command_resources.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/output_image_resources.h"
#include "frame/vulkan/pipeline_resources.h"
#include "frame/vulkan/raytrace_scene_renderer.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/sync_resources.h"

namespace frame::vulkan
{

Renderer::Renderer(Device& device)
    : device_(device),
      raytrace_scene_renderer_(std::make_unique<RaytraceSceneRenderer>(device))
{
}

Renderer::~Renderer() = default;

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
    RecordCommandBuffer(command_buffer, image_index);

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

void Renderer::RecordCommandBuffer(
    vk::CommandBuffer command_buffer,
    std::uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info;
    command_buffer.begin(begin_info);

    const auto extent = device_.swapchain_resources_->GetExtent();
    const auto& images = device_.swapchain_resources_->GetImages();
    const auto& render_pass = device_.swapchain_resources_->GetRenderPass();
    const auto& framebuffers = device_.swapchain_resources_->GetFramebuffers();
    const auto& gui_render_pass =
        device_.swapchain_resources_->GetGuiRenderPass();
    const auto& gui_framebuffers =
        device_.swapchain_resources_->GetGuiFramebuffers();

    const SceneState scene_state = device_.BuildFrameSceneState(extent);
    raytrace_scene_renderer_->UpdateUniformBuffer(scene_state);
    raytrace_scene_renderer_->Render(command_buffer, extent);

    std::array<vk::ClearValue, 1> clear_values{};
    clear_values[0].color = vk::ClearColorValue(
        std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});

    auto draw_scene = [&]() -> bool {
        if (!device_.pipeline_resources_ ||
            !device_.pipeline_resources_->HasGraphicsPipeline())
        {
            return false;
        }
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            device_.pipeline_resources_->GetGraphicsPipeline());

        vk::Viewport viewport(
            0.0f,
            0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f,
            1.0f);
        command_buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor({0, 0}, extent);
        command_buffer.setScissor(0, 1, &scissor);

        if (device_.descriptor_set_layout_ && device_.descriptor_set_)
        {
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                device_.pipeline_resources_->GetGraphicsPipelineLayout(),
                0,
                device_.descriptor_set_,
                {});
        }

        glm::mat4 projection = scene_state.projection;
        glm::mat4 view = scene_state.view;
        glm::mat4 model = scene_state.model;

        const bool needs_scene_matrices =
            device_.pipeline_resources_->GetPushConstantSize() > 0 &&
            !(device_.pipeline_resources_->UsesProceduralQuadPipeline() &&
              device_.active_program_info_ &&
              device_.active_program_info_->uses_time_uniform);

        if (needs_scene_matrices && device_.level_)
        {
            try
            {
                Camera camera_for_frame(device_.level_->GetDefaultCamera());
                auto camera_holder_id = device_.level_->GetDefaultCameraId();
                if (camera_holder_id != NullId)
                {
                    auto& node =
                        device_.level_->GetSceneNodeFromId(camera_holder_id);
                    auto matrix_node = node.GetLocalModel(
                        static_cast<double>(device_.elapsed_time_seconds_));
                    auto inverse_model = glm::inverse(matrix_node);
                    camera_for_frame.SetFront(
                        device_.level_->GetDefaultCamera().GetFront() *
                        glm::mat3(inverse_model));
                    camera_for_frame.SetPosition(
                        glm::vec3(
                            glm::vec4(
                                device_.level_->GetDefaultCamera().GetPosition(),
                                1.0f) *
                            inverse_model));
                }

                if (extent.height != 0)
                {
                    camera_for_frame.SetAspectRatio(
                        static_cast<float>(extent.width) /
                        static_cast<float>(extent.height));
                }
                projection = camera_for_frame.ComputeProjection();
                projection[1][1] *= -1.0f;
                view = camera_for_frame.ComputeView();
                glm::mat4 rotation = glm::mat4(1.0f);
                view = rotation * view;

                const auto mesh_pairs = device_.level_->GetMeshMaterialIds();
                if (!mesh_pairs.empty())
                {
                    auto node_id = mesh_pairs.front().first;
                    auto& node = device_.level_->GetSceneNodeFromId(node_id);
                    model = node.GetLocalModel(
                        static_cast<double>(device_.elapsed_time_seconds_));
                }
            }
            catch (const std::exception& ex)
            {
                device_.logger_->warn(
                    "Failed to compute scene matrices: {}", ex.what());
            }
        }

        if (device_.pipeline_resources_->GetPushConstantSize() > 0)
        {
            if (device_.pipeline_resources_->UsesProceduralQuadPipeline() &&
                device_.active_program_info_ &&
                device_.active_program_info_->uses_time_uniform)
            {
                float time = device_.elapsed_time_seconds_;
                command_buffer.pushConstants(
                    device_.pipeline_resources_->GetGraphicsPipelineLayout(),
                    device_.pipeline_resources_->GetPushConstantStages(),
                    0,
                    device_.pipeline_resources_->GetPushConstantSize(),
                    &time);
            }
            else
            {
                struct alignas(16) PushConstants
                {
                    glm::mat4 projection;
                    glm::mat4 view;
                    glm::mat4 model;
                    float time_s;
                } push_constants{
                    projection,
                    view,
                    model,
                    device_.elapsed_time_seconds_};
                command_buffer.pushConstants(
                    device_.pipeline_resources_->GetGraphicsPipelineLayout(),
                    device_.pipeline_resources_->GetPushConstantStages(),
                    0,
                    device_.pipeline_resources_->GetPushConstantSize(),
                    &push_constants);
            }
        }

        if (device_.pipeline_resources_->UsesProceduralQuadPipeline())
        {
            command_buffer.draw(6, 1, 0, 0);
            return true;
        }
        if (device_.mesh_resources_ && !device_.mesh_resources_->Empty())
        {
            const auto& mesh = device_.mesh_resources_->GetMeshes().front();
            const vk::DeviceSize offsets[] = {0};
            command_buffer.bindVertexBuffers(0, *mesh.vertex_buffer, offsets);
            if (mesh.index_buffer)
            {
                command_buffer.bindIndexBuffer(
                    *mesh.index_buffer,
                    0,
                    vk::IndexType::eUint32);
                command_buffer.drawIndexed(mesh.index_count, 1, 0, 0, 0);
            }
            else
            {
                command_buffer.draw(mesh.index_count, 1, 0, 0);
            }
            return true;
        }
        return false;
    };

    bool scene_pass_executed = false;
    bool scene_content_rendered = false;
    if (render_pass && image_index < framebuffers.size())
    {
        vk::RenderPassBeginInfo render_pass_info(
            *render_pass,
            *framebuffers[image_index],
            vk::Rect2D({0, 0}, extent),
            static_cast<std::uint32_t>(clear_values.size()),
            clear_values.data());

        command_buffer.beginRenderPass(
            render_pass_info,
            vk::SubpassContents::eInline);
        scene_content_rendered = draw_scene();
        command_buffer.endRenderPass();
        scene_pass_executed = true;
    }

    if (!scene_pass_executed && image_index < images.size())
    {
        const vk::Image swapchain_image = images[image_index];
        vk::ImageMemoryBarrier to_transfer_src(
            {},
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain_image,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            to_transfer_src);
    }

    if (device_.output_image_resources_ &&
        device_.output_image_resources_->HasSwapchainPreviewImage() &&
        scene_content_rendered &&
        image_index < images.size() &&
        extent.width > 0 && extent.height > 0)
    {
        const vk::Image swapchain_image = images[image_index];

        std::array<vk::ImageMemoryBarrier, 2> to_copy_barriers = {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                swapchain_image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
            vk::ImageMemoryBarrier(
                device_.output_image_resources_->IsSwapchainPreviewInShaderRead()
                    ? vk::AccessFlagBits::eShaderRead
                    : vk::AccessFlags{},
                vk::AccessFlagBits::eTransferWrite,
                device_.output_image_resources_->IsSwapchainPreviewInShaderRead()
                    ? vk::ImageLayout::eShaderReadOnlyOptimal
                    : vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                device_.output_image_resources_->GetSwapchainPreviewImage(),
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})};

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            to_copy_barriers);

        vk::ImageCopy copy_region(
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {0, 0, 0},
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {0, 0, 0},
            {extent.width, extent.height, 1});
        command_buffer.copyImage(
            swapchain_image,
            vk::ImageLayout::eTransferSrcOptimal,
            device_.output_image_resources_->GetSwapchainPreviewImage(),
            vk::ImageLayout::eTransferDstOptimal,
            copy_region);

        std::array<vk::ImageMemoryBarrier, 2> from_copy_barriers = {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferRead,
                device_.gui_render_callback_
                    ? vk::AccessFlagBits::eTransferRead
                    : vk::AccessFlags{},
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                swapchain_image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                device_.output_image_resources_->GetSwapchainPreviewImage(),
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})};

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAllCommands,
            {},
            nullptr,
            nullptr,
            from_copy_barriers);

        device_.output_image_resources_->SetSwapchainPreviewInShaderRead(true);
    }

    if (device_.gui_render_callback_ && gui_render_pass &&
        image_index < gui_framebuffers.size())
    {
        vk::RenderPassBeginInfo gui_pass_info(
            *gui_render_pass,
            *gui_framebuffers[image_index],
            vk::Rect2D({0, 0}, extent),
            0,
            nullptr);
        command_buffer.beginRenderPass(
            gui_pass_info,
            vk::SubpassContents::eInline);
        try
        {
            device_.gui_render_callback_(command_buffer);
        }
        catch (const std::exception& ex)
        {
            device_.logger_->error("Failed to render Vulkan GUI: {}", ex.what());
        }
        command_buffer.endRenderPass();
    }
    else if (image_index < images.size())
    {
        const vk::Image swapchain_image = images[image_index];
        vk::ImageMemoryBarrier to_present(
            vk::AccessFlagBits::eTransferRead,
            {},
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain_image,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {},
            nullptr,
            nullptr,
            to_present);
    }

    command_buffer.end();
}

} // namespace frame::vulkan
