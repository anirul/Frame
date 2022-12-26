#include "frame/vulkan/sdl_vulkan_none.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace frame::vulkan {

SDLVulkanNone::SDLVulkanNone(glm::uvec2 size) : size_(size){
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

    for (const auto& extension : extensions) {
        logger_->info("Extension: {}", extension);
    }

    VkApplicationInfo app_info  = {};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "SDL Vulkan Headless";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "Frame";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info    = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance.");
    }
}

SDLVulkanNone::~SDLVulkanNone() {
    // Destroy the surface and instance when finished
    vkDestroyInstance(instance_, nullptr);
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

void* SDLVulkanNone::GetGraphicContext() const {
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
    if (physical_device_count == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance_, &physical_device_count, physical_devices.data());
    if (physical_device_count == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");
    }

    for (const auto& physical_device : physical_devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device, &properties);
        logger_->info("Device: {}", properties.deviceName);
    }
    return instance_;
}

}  // End namespace frame::vulkan.
