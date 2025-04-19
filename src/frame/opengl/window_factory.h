#pragma once

#include <GL/glew.h>

#include <memory>
#include <utility>

#include "frame/window_interface.h"

namespace frame::opengl
{

/**
 * @brief Create an instance of the window in SDL using OpenGL.
 * @param size: Window size.
 * @return A unique pointer to the window object.
 */
std::unique_ptr<WindowInterface> CreateSDLOpenGLWindow(glm::uvec2 size);
/*
 * @brief Create a non window using OpenGL (mostly used for testing).
 * @param size: Size of the output image.
 * @return A unique pointer to a fake window object.
 */
std::unique_ptr<WindowInterface> CreateSDLOpenGLNone(glm::uvec2 size);

} // namespace frame::opengl
