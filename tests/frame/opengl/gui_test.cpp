#include <gtest/gtest.h>

#include <SDL3/SDL_keycode.h>

#include "frame/opengl/sdl_opengl_window.h"

namespace test
{

TEST(OpenGLGuiTest, ToggleKeyMatchesF11)
{
    EXPECT_TRUE(frame::opengl::SDLOpenGLWindow::IsGuiToggleKey(SDLK_F11));
    EXPECT_FALSE(frame::opengl::SDLOpenGLWindow::IsGuiToggleKey(SDLK_ESCAPE));
}

} // namespace test
