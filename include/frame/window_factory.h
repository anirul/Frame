#pragma once

#include "frame/api.h"
#include "frame/window_interface.h"

namespace frame {

/**
 * @brief Create a new window.
 * This could not be named create window as windows is already defining it as a macro.
 * @param drawing_target_enum: Window or not?
 * @param rendering_api_enum: Which rendering API it use [OpenGL, Vulkan, ...].
 * @param size: The size of the window.
 * @return A unique pointer to a window.
 */
std::unique_ptr<WindowInterface> CreateNewWindow(
    DrawingTargetEnum drawing_target_enum = DrawingTargetEnum::WINDOW,
    RenderingAPIEnum rendering_api_enum = RenderingAPIEnum::OPENGL, glm::uvec2 size = { 320, 200 });

}  // End namespace frame.
