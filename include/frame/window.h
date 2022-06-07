#pragma once

#include <memory>
#include <utility>

#include "frame/window_interface.h"

namespace frame {

/**
 * @brief Create an instance of the window in SDL using OpenGL.
 * @param size: Window size.
 * @return A unique pointer to the window object.
 */
std::unique_ptr<WindowInterface> CreateSDLOpenGL(std::pair<std::uint32_t, std::uint32_t> size);

}  // End namespace frame.
