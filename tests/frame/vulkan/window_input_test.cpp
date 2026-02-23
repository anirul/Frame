#include <gtest/gtest.h>

#include <SDL3/SDL_keycode.h>

#include "frame/vulkan/sdl_vulkan_window.h"

namespace test
{

TEST(VulkanWindowInputTest, GuiToggleKeyMatchesOpenGLBehavior)
{
    EXPECT_TRUE(frame::vulkan::SDLVulkanWindow::IsGuiToggleKey(SDLK_F11));
    EXPECT_FALSE(frame::vulkan::SDLVulkanWindow::IsGuiToggleKey(SDLK_ESCAPE));
}

} // namespace test
