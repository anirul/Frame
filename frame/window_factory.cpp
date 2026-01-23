#include "frame/window_factory.h"

#include <atomic>
#include <memory>
#include <stdexcept>

#include "frame/api.h"
#include "frame/opengl/window_factory.h"

namespace
{

std::atomic<frame::VulkanWindowFactory::FactoryFn> g_create_vulkan_window =
    nullptr;
std::atomic<frame::VulkanWindowFactory::FactoryFn> g_create_vulkan_none =
    nullptr;

} // namespace

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
        case RenderingAPIEnum::VULKAN: {
            auto factory =
                g_create_vulkan_none.load(std::memory_order_acquire);
            if (!factory)
            {
                throw std::runtime_error("Vulkan window factory not registered.");
            }
            return factory(size);
        }
        default:
            throw std::runtime_error("Unsupported device enum.");
        }
    case DrawingTargetEnum::WINDOW:
        switch (rendering_api_enum)
        {
        case RenderingAPIEnum::OPENGL:
            return frame::opengl::CreateSDLOpenGLWindow(size);
        case RenderingAPIEnum::VULKAN: {
            auto factory =
                g_create_vulkan_window.load(std::memory_order_acquire);
            if (!factory)
            {
                throw std::runtime_error("Vulkan window factory not registered.");
            }
            return factory(size);
        }
        default:
            throw std::runtime_error("Unsupported device enum.");
        }
    default:
        throw std::runtime_error("Unsupported window enum.");
    }
}

void RegisterVulkanWindowFactory(VulkanWindowFactory factory)
{
    g_create_vulkan_window.store(
        factory.create_window, std::memory_order_release);
    g_create_vulkan_none.store(factory.create_none, std::memory_order_release);
}

bool HasVulkanWindowFactory()
{
    return g_create_vulkan_window.load(std::memory_order_acquire) != nullptr &&
           g_create_vulkan_none.load(std::memory_order_acquire) != nullptr;
}

} // End namespace frame.
