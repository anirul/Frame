#include "frame/window_factory.h"

#include <memory>

#include "frame/api.h"
#include "frame/opengl/window.h"

namespace frame {

/**
 * @brief Create a new window.
 * This could not be named create window as windows is already defining it as a macro.
 * @param window_enum: The window API you want to use.
 * @param device_enum: The device API you want to use.
 * @param size: The size of the window.
 * @return A unique pointer to a window.
 */
std::unique_ptr<frame::WindowInterface> CreateNewWindow(
    WindowEnum window_enum, DeviceEnum device_enum, std::pair<std::uint32_t, std::uint32_t> size) {
    switch (window_enum) {
        case WindowEnum::NONE:
            switch (device_enum) {
                case DeviceEnum::OPENGL:
                    return frame::opengl::CreateNoneOpenGL(size);
                default:
                    throw std::runtime_error("Unsupported device enum.");
            }
        case WindowEnum::SDL2:
            switch (device_enum) {
                case DeviceEnum::OPENGL:
                    return frame::opengl::CreateSDL2OpenGL(size);
                default:
                    throw std::runtime_error("Unsupported device enum.");
            }
        default:
            throw std::runtime_error("Unsupported window enum.");
    }
}

}  // End namespace frame.
