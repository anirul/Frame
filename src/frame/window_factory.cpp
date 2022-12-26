#include "frame/window_factory.h"

#include <memory>

#include "frame/api.h"
#include "frame/opengl/window_factory.h"
#include "frame/vulkan/window_factory.h"

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
    DrawingTargetEnum drawing_target_enum /* = DrawingTargetEnum::WINDOW*/,
    RenderingAPIEnum rendering_api_enum /*   = RenderingAPIEnum::OPENGL*/,
    glm::uvec2 size /*                       = { 320, 200 }*/) {
    switch (drawing_target_enum) {
        case DrawingTargetEnum::NONE:
            switch (rendering_api_enum) {
                case RenderingAPIEnum::OPENGL:
                    return frame::opengl::CreateSDL2OpenGLNone(size);
                case RenderingAPIEnum::VULKAN:
                    return frame::vulkan::CreateSDL2VulkanNone(size);
                default:
                    throw std::runtime_error("Unsupported device enum.");
            }
        case DrawingTargetEnum::WINDOW:
            switch (rendering_api_enum) {
                case RenderingAPIEnum::OPENGL:
                    return frame::opengl::CreateSDL2OpenGLWindow(size);
                case RenderingAPIEnum::VULKAN:
                    return frame::vulkan::CreateSDL2VulkanWindow(size);
                default:
                    throw std::runtime_error("Unsupported device enum.");
            }
        default:
            throw std::runtime_error("Unsupported window enum.");
    }
}

}  // End namespace frame.
