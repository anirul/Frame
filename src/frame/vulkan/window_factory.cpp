#include "frame/vulkan/window_factory.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "frame/vulkan/device.h"
#include "frame/vulkan/sdl_vulkan_none.h"
#include "frame/vulkan/sdl_vulkan_window.h"

namespace frame::vulkan
{

std::unique_ptr<WindowInterface> CreateSDL2VulkanWindow(glm::uvec2 size)
{
    auto device = std::make_unique<vulkan::Device>(size);
    auto window = std::make_unique<vulkan::SDLVulkanWindow>(size);
    window->SetUniqueDevice(std::move(device));
    return window;
}

std::unique_ptr<WindowInterface> CreateSDL2VulkanNone(glm::uvec2 size)
{
    auto device = std::make_unique<vulkan::Device>(size);
    auto window = std::make_unique<vulkan::SDLVulkanNone>(size);
    window->SetUniqueDevice(std::move(device));
    return window;
}

} // End namespace frame::vulkan.
