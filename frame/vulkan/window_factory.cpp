#include "frame/vulkan/window_factory.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "frame/window_factory.h"
#include "frame/vulkan/device.h"
#include "frame/vulkan/sdl_vulkan_none.h"
#include "frame/vulkan/sdl_vulkan_window.h"

namespace frame::vulkan
{

std::unique_ptr<WindowInterface> CreateSDLVulkanWindow(glm::uvec2 size)
{
    auto window = std::make_unique<SDLVulkanWindow>(size);
    auto context = window->GetGraphicContext();
    auto& surface = window->GetVulkanSurfaceKHR();
    if (!context)
    {
        return nullptr;
    }
    window->SetUniqueDevice(std::make_unique<Device>(context, size, surface));
    return window;
}

std::unique_ptr<WindowInterface> CreateSDLVulkanNone(glm::uvec2 size)
{
    auto window = std::make_unique<SDLVulkanNone>(size);
    auto context = window->GetGraphicContext();
    auto& surface = window->GetVulkanSurfaceKHR();
    if (!context)
    {
        return nullptr;
    }
    window->SetUniqueDevice(std::make_unique<Device>(context, size, surface));
    return window;
}

} // End namespace frame::vulkan.

namespace
{

std::once_flag g_register_vulkan_factory_once;

} // namespace

namespace frame::vulkan
{

void EnsureWindowFactoryRegistered()
{
    std::call_once(g_register_vulkan_factory_once, [] {
        frame::RegisterVulkanWindowFactory(
            frame::VulkanWindowFactory{
                frame::vulkan::CreateSDLVulkanWindow,
                frame::vulkan::CreateSDLVulkanNone});
    });
}

} // namespace frame::vulkan
