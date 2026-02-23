#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

#include "frame/logger.h"
#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class SwapchainResources
{
  public:
    static constexpr vk::ImageLayout ScenePassInitialLayout()
    {
        return vk::ImageLayout::ePresentSrcKHR;
    }
    static constexpr vk::ImageLayout ScenePassFinalLayout()
    {
        return vk::ImageLayout::eTransferSrcOptimal;
    }
    static constexpr vk::ImageLayout GuiPassInitialLayout()
    {
        return vk::ImageLayout::eTransferSrcOptimal;
    }
    static constexpr vk::ImageLayout GuiPassFinalLayout()
    {
        return vk::ImageLayout::ePresentSrcKHR;
    }

    SwapchainResources(
        vk::PhysicalDevice physical_device,
        vk::Device device,
        vk::SurfaceKHR surface,
        std::uint32_t graphics_queue_family_index,
        std::uint32_t present_queue_family_index,
        const Logger& logger);

    void Create(glm::uvec2 size);
    void Destroy();
    void Recreate(glm::uvec2 size);

    bool IsValid() const
    {
        return static_cast<bool>(swapchain_);
    }

    const vk::UniqueSwapchainKHR& GetSwapchain() const
    {
        return swapchain_;
    }

    const std::vector<vk::Image>& GetImages() const
    {
        return images_;
    }

    const std::vector<vk::UniqueImageView>& GetImageViews() const
    {
        return image_views_;
    }

    const vk::UniqueRenderPass& GetRenderPass() const
    {
        return render_pass_;
    }
    const vk::UniqueRenderPass& GetGuiRenderPass() const
    {
        return gui_render_pass_;
    }

    const std::vector<vk::UniqueFramebuffer>& GetFramebuffers() const
    {
        return framebuffers_;
    }
    const std::vector<vk::UniqueFramebuffer>& GetGuiFramebuffers() const
    {
        return gui_framebuffers_;
    }

    vk::Extent2D GetExtent() const
    {
        return extent_;
    }

    vk::Format GetImageFormat() const
    {
        return image_format_;
    }

  private:
    vk::SurfaceFormatKHR SelectSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& formats) const;
    vk::PresentModeKHR SelectPresentMode(
        const std::vector<vk::PresentModeKHR>& modes) const;
    vk::Extent2D SelectSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities,
        glm::uvec2 size) const;

    vk::PhysicalDevice physical_device_;
    vk::Device device_;
    vk::SurfaceKHR surface_;
    std::uint32_t graphics_queue_family_index_ = 0;
    std::uint32_t present_queue_family_index_ = 0;
    const Logger& logger_;
    vk::Format image_format_ = vk::Format::eUndefined;
    vk::Extent2D extent_{};
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> images_;
    std::vector<vk::UniqueImageView> image_views_;
    vk::UniqueRenderPass render_pass_;
    std::vector<vk::UniqueFramebuffer> framebuffers_;
    vk::UniqueRenderPass gui_render_pass_;
    std::vector<vk::UniqueFramebuffer> gui_framebuffers_;
};

} // namespace frame::vulkan
