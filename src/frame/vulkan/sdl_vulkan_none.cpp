#include "frame/vulkan/sdl_vulkan_none.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#include "frame/vulkan/debug_callback.h"

namespace frame::vulkan {

SDLVulkanNone::SDLVulkanNone(glm::uvec2 size) : size_(size) {
    // Initialize SDL with the video subsystem.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(fmt::format("Couldn't initialize SDL: {}", SDL_GetError()));
    }

    if (SDL_Vulkan_LoadLibrary(nullptr) == -1) {
        throw std::runtime_error(fmt::format("Couldn't load Vulkan library: {}", SDL_GetError()));
    }

    // Create an SDL window to use as a surface for Vulkan.
    sdl_window_ =
        SDL_CreateWindow("Vulkan Headless", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         size_.x, size_.y, SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
    if (!sdl_window_) {
        throw std::runtime_error(fmt::format("Couldn't initialize window: {}", SDL_GetError()));
    }

    // Get the required extensions for creating a Vulkan surface.
    uint32_t extension_count = 0;
    SDL_Vulkan_GetInstanceExtensions(sdl_window_, &extension_count, nullptr);
    if (extension_count == 0) {
        throw std::runtime_error(
            fmt::format("Could not get the extension count: {}", SDL_GetError()));
    }
    std::vector<const char*> extensions(extension_count);
    SDL_Vulkan_GetInstanceExtensions(sdl_window_, &extension_count, extensions.data());
    if (extension_count == 0) {
        throw std::runtime_error(
            fmt::format("Could not get the extension count: {}", SDL_GetError()));
    }
    // Add the extension to have the debug layers.
    extensions.push_back("VK_EXT_debug_utils");
    for (const auto& extension : extensions) {
        logger_->info("Extension: {}", extension);
    }

    vk::ApplicationInfo application_info("Frame", VK_MAKE_VERSION(0, 5, 1), "SDL - Vulkan - None",
                                         VK_MAKE_VERSION(0, 5, 1), VK_API_VERSION_1_3);
    vk::InstanceCreateInfo instance_create_info({}, &application_info, 0, nullptr,
                                                static_cast<std::uint32_t>(extensions.size()),
                                                extensions.data());

    vk_unique_instance_ = vk::createInstanceUnique(instance_create_info);
    vk_dispatch_loader_dynamic_.init(*vk_unique_instance_, vkGetInstanceProcAddr);
    auto result = vk_unique_instance_->createDebugUtilsMessengerEXT(
        vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            DebugCallback),
        nullptr, vk_dispatch_loader_dynamic_);
}

SDLVulkanNone::~SDLVulkanNone() {
    // Destroy the surface and instance when finished
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLVulkanNone::Run() {
    for (const auto& plugin_interface : device_->GetPluginPtrs()) {
        plugin_interface->Startup(size_);
    }
    if (input_interface_) input_interface_->NextFrame();
    device_->Display(0.0);
    for (const auto& plugin_interface : device_->GetPluginPtrs()) {
        plugin_interface->Update(device_.get(), 0.0);
    }
}

void* SDLVulkanNone::GetGraphicContext() const { return vk_unique_instance_.get(); }

}  // End namespace frame::vulkan.
