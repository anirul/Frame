#include "frame/vulkan/sdl_vulkan_none.h"

#include "frame/vulkan/debug_callback.h"
#include <SDL2/SDL_vulkan.h>

namespace frame::vulkan
{

SDLVulkanNone::SDLVulkanNone(glm::uvec2 size) : size_(size)
{
    // Initialize SDL with the video subsystem.
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error(
            fmt::format("Couldn't initialize SDL: {}", SDL_GetError()));
    }

    if (SDL_Vulkan_LoadLibrary(nullptr) == -1)
    {
        throw std::runtime_error(
            fmt::format("Couldn't load Vulkan library: {}", SDL_GetError()));
    }

    // Create an SDL window to use as a surface for Vulkan.
    sdl_window_ = SDL_CreateWindow(
        "Vulkan Headless",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        size_.x,
        size_.y,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
    if (!sdl_window_)
    {
        throw std::runtime_error(
            fmt::format("Couldn't initialize window: {}", SDL_GetError()));
    }

    std::vector<const char*> sdlExtensions;
    unsigned int sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(sdl_window_, &sdlExtensionCount, nullptr);
    sdlExtensions.resize(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(
        sdl_window_, &sdlExtensionCount, sdlExtensions.data());

    vk::ApplicationInfo appInfo(
        "Frame Vulkan",
        VK_MAKE_VERSION(1, 0, 0),
        "Frame (SDL Vulkan None)",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3);

    vk::InstanceCreateInfo instanceCreateInfo(
        {},
        &appInfo,
        0,
        nullptr,
        static_cast<uint32_t>(sdlExtensions.size()),
        sdlExtensions.data());

    vk_instance_ = vk::raii::Instance(vk_context_, instanceCreateInfo);

    // Select Physical Device
    std::vector<vk::raii::PhysicalDevice> physicalDevices =
        vk_instance_.value().enumeratePhysicalDevices();
    if (physicalDevices.empty())
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    vk::raii::PhysicalDevice& physicalDevice = physicalDevices[0];
}

SDLVulkanNone::~SDLVulkanNone()
{
    // Destroy the surface and instance when finished.
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLVulkanNone::Run(std::function<void()> /* lambda*/)
{
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Startup(size_);
    }
    if (input_interface_)
        input_interface_->NextFrame();
    device_->Display(0.0);
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Update(*device_.get(), 0.0);
    }
}

void* SDLVulkanNone::GetGraphicContext() const
{
    if (vk_instance_)
    {
        return vk_instance_.value().operator*();
    }
    throw std::runtime_error(
        "Try to access an uninitialized pointer to a vulkan instance.");
}

} // End namespace frame::vulkan.
