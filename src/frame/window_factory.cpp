#include "frame/window_factory.h"

#include <memory>

#include "frame/api.h"
#include "frame/opengl/window_factory.h"
#ifdef FRAME_HAS_VULKAN
#include "frame/vulkan/window_factory.h"
#endif

namespace frame
{

std::unique_ptr<frame::WindowInterface> CreateNewWindow(
    DrawingTargetEnum drawing_target_enum /* = DrawingTargetEnum::WINDOW*/,
    RenderingAPIEnum rendering_api_enum /*   = RenderingAPIEnum::OPENGL*/,
    glm::uvec2 size /*                       = { 320, 200 }*/)
{
    switch (drawing_target_enum)
    {
    case DrawingTargetEnum::NONE:
        switch (rendering_api_enum)
        {
        case RenderingAPIEnum::OPENGL:
            return frame::opengl::CreateSDLOpenGLNone(size);
#ifdef FRAME_HAS_VULKAN
        case RenderingAPIEnum::VULKAN:
            return frame::vulkan::CreateSDLVulkanNone(size);
#endif
        default:
            throw std::runtime_error("Unsupported device enum.");
        }
    case DrawingTargetEnum::WINDOW:
        switch (rendering_api_enum)
        {
        case RenderingAPIEnum::OPENGL:
            return frame::opengl::CreateSDLOpenGLWindow(size);
#ifdef FRAME_HAS_VULKAN
        case RenderingAPIEnum::VULKAN:
            return frame::vulkan::CreateSDLVulkanWindow(size);
#endif
        default:
            throw std::runtime_error("Unsupported device enum.");
        }
    default:
        throw std::runtime_error("Unsupported window enum.");
    }
}

} // End namespace frame.
