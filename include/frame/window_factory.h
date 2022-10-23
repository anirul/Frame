#pragma once

#include "frame/api.h"
#include "frame/window_interface.h"

namespace frame {

/**
 * @brief Create a new window.
 * This could not be named create window as windows is already defining it as a macro.
 * @param window_enum: The window API you want to use [NONE, SDL2, ...].
 * @param device_enum: The device API you want to use [OPENGL, ...].
 * @param size: The size of the window.
 * @return A unique pointer to a window.
 */
std::unique_ptr<WindowInterface> CreateNewWindow(WindowEnum window_enum, DeviceEnum device_enum,
                                                 std::pair<std::uint32_t, std::uint32_t> size);

}  // End namespace frame.
