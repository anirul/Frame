#include <gtest/gtest.h>

#include <SDL3/SDL_keycode.h>

#include "frame/vulkan/sdl_vulkan_window.h"
#include "frame/vulkan/swapchain_resources.h"

namespace test
{

TEST(VulkanGuiTest, ToggleKeyMatchesF11)
{
    EXPECT_TRUE(frame::vulkan::SDLVulkanWindow::IsGuiToggleKey(SDLK_F11));
    EXPECT_FALSE(frame::vulkan::SDLVulkanWindow::IsGuiToggleKey(SDLK_ESCAPE));
}

TEST(VulkanGuiTest, SwapchainGuiStackLayoutsAreStable)
{
    EXPECT_EQ(
        frame::vulkan::SwapchainResources::ScenePassInitialLayout(),
        vk::ImageLayout::ePresentSrcKHR);
    EXPECT_EQ(
        frame::vulkan::SwapchainResources::ScenePassFinalLayout(),
        vk::ImageLayout::eTransferSrcOptimal);
    EXPECT_EQ(
        frame::vulkan::SwapchainResources::GuiPassInitialLayout(),
        vk::ImageLayout::eTransferSrcOptimal);
    EXPECT_EQ(
        frame::vulkan::SwapchainResources::GuiPassFinalLayout(),
        vk::ImageLayout::ePresentSrcKHR);
}

} // namespace test
