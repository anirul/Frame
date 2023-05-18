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

namespace frame::vulkan {

std::unique_ptr<WindowInterface> CreateSDL2VulkanWindow(glm::uvec2 size) {
  auto window = std::make_unique<SDLVulkanWindow>(size);
  auto context = window->GetGraphicContext();
  auto& dispatch = window->GetVulkanDispatch();
  auto& surface = window->GetVulkanSurfaceKHR();
  if (!context) return nullptr;
  window->SetUniqueDevice(
      std::make_unique<Device>(context, size, surface, dispatch));
  return window;
}

std::unique_ptr<WindowInterface> CreateSDL2VulkanNone(glm::uvec2 size) {
  auto window = std::make_unique<SDLVulkanNone>(size);
  auto context = window->GetGraphicContext();
  auto& dispatch = window->GetVulkanDispatch();
  auto& surface = window->GetVulkanSurfaceKHR();
  if (!context) return nullptr;
  window->SetUniqueDevice(
      std::make_unique<Device>(context, size, surface, dispatch));
  return window;
}

}  // End namespace frame::vulkan.
