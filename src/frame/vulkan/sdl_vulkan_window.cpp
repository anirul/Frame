#include "frame/vulkan/sdl_vulkan_window.h"

namespace frame::vulkan {

SDLVulkanWindow::SDLVulkanWindow(glm::uvec2 size) { throw std::runtime_error("Not implemented!"); }

SDLVulkanWindow::~SDLVulkanWindow() {}

void SDLVulkanWindow::Run() { throw std::runtime_error("Not implemented!"); }

void* SDLVulkanWindow::GetGraphicContext() const { throw std::runtime_error("Not implemented!"); }

void SDLVulkanWindow::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) {
    throw std::runtime_error("Not implemented!");
}

frame::FullScreenEnum SDLVulkanWindow::GetFullScreenEnum() const {
    throw std::runtime_error("Not implemented!");
}

bool SDLVulkanWindow::RunEvent(const SDL_Event& event, const double dt) {
    throw std::runtime_error("Not implemented!");
}

const char SDLVulkanWindow::SDLButtonToChar(const Uint8 button) const {
    throw std::runtime_error("Not implemented!");
}

const double SDLVulkanWindow::GetFrameDt(const double t) const {
    throw std::runtime_error("Not implemented!");
}

}  // End namespace frame::vulkan.
