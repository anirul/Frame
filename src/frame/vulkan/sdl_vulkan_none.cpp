#include "frame/vulkan/sdl_vulkan_none.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "frame/vulkan/debug_callback.h"

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

    // Create an SDL window to use as a surface for Vulkan.
    sdl_window_ = SDL_CreateWindow(
        "Vulkan Headless",
        size_.x,
        size_.y,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
    if (!sdl_window_)
    {
        throw std::runtime_error(
            fmt::format("Couldn't initialize window: {}", SDL_GetError()));
    }

    // Get the required extensions for creating a Vulkan surface.
    uint32_t extension_count = 0;
    std::vector<const char*> extensions;
    const char* const* extention_c =
        SDL_Vulkan_GetInstanceExtensions(&extension_count);
    for (uint32_t i = 0; i < extension_count; i++)
    {
        extensions.push_back(extention_c[i]);
    }
    if (extension_count == 0)
    {
        throw std::runtime_error(fmt::format(
            "Could not get the extension count: {}", SDL_GetError()));
    }

#ifdef VK_EXT_ENABLE_DEBUG_EXTENSION
    // Add the extension to have the debug layers.
    extensions.push_back("VK_EXT_debug_utils");
#endif
    for (const auto& extension : extensions)
    {
        logger_->info("Extension: {}", extension);
    }

    vk::ApplicationInfo application_info(
        "Frame",
        VK_MAKE_VERSION(0, 5, 1),
        "SDL - Vulkan - None",
        VK_MAKE_VERSION(0, 5, 1),
        VK_API_VERSION_1_3);
    vk::InstanceCreateInfo instance_create_info(
        {},
        &application_info,
        0,
        nullptr,
        static_cast<std::uint32_t>(extensions.size()),
        extensions.data());

    vk_unique_instance_ = vk::createInstanceUnique(instance_create_info);
#ifdef VK_EXT_ENABLE_DEBUG_EXTENSION
    auto vk_debug_utils_messenger_create_info =
        vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            vk::PFN_DebugUtilsMessengerCallbackEXT(DebugCallback));
    auto result =
        vk_unique_instance_->createDebugUtilsMessengerEXT(
            vk_debug_utils_messenger_create_info);
#endif

    // Create the surface.
    // CHECKME(anirul): What happen in case of a resize?
    VkSurfaceKHR vk_surface;
    if (!SDL_Vulkan_CreateSurface(
            sdl_window_, *vk_unique_instance_, nullptr, &vk_surface))
    {
        throw std::runtime_error(fmt::format(
            "Error while create vulkan surface: {}", SDL_GetError()));
    }
    vk_surface_ = vk::UniqueSurfaceKHR(vk_surface);
}

SDLVulkanNone::~SDLVulkanNone()
{
    // Has to be reset before closing the window.
    vk_unique_instance_.reset();
    vk_surface_.reset();
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
    return vk_unique_instance_.get();
}

} // End namespace frame::vulkan.
