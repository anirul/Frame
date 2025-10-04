#include "frame/vulkan/sdl_vulkan_none.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <format>
#include <string>
#include <vector>

#include "frame/vulkan/debug_callback.h"

namespace frame::vulkan
{

SDLVulkanNone::SDLVulkanNone(glm::uvec2 size) : size_(size)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(
            std::format("Couldn't initialize SDL: {}", SDL_GetError()));
    }

    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        throw std::runtime_error(
            std::format("Couldn't load Vulkan library: {}", SDL_GetError()));
    }

    const auto vk_get_instance_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(        SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (!vk_get_instance_proc_addr)
    {
        throw std::runtime_error(std::format("Couldn't resolve vkGetInstanceProcAddr: {}", SDL_GetError()));
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

    const Uint32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN;
    sdl_window_ = SDL_CreateWindow("Vulkan Headless", size_.x, size_.y, window_flags);
    if (!sdl_window_)
    {
        throw std::runtime_error(
            std::format("Couldn't initialize window: {}", SDL_GetError()));
    }

    Uint32 extension_count = 0;
    const char* const* extension_names =
        SDL_Vulkan_GetInstanceExtensions(&extension_count);
    if (!extension_names || extension_count == 0)
    {
        throw std::runtime_error(std::format(
            "Could not query Vulkan instance extensions: {}",
            SDL_GetError()));
    }

    std::vector<const char*> extensions;
    extensions.reserve(static_cast<std::size_t>(extension_count) + 1);
    extensions.insert(
        extensions.end(), extension_names, extension_names + extension_count);
#ifdef VK_EXT_ENABLE_DEBUG_EXTENSION
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
        VK_API_VERSION_1_4);
    vk::InstanceCreateInfo instance_create_info(
        {},
        &application_info,
        0,
        nullptr,
        static_cast<std::uint32_t>(extensions.size()),
        extensions.data());

    vk_unique_instance_ = vk::createInstanceUnique(instance_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*vk_unique_instance_, vk_get_instance_proc_addr);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(
            sdl_window_,
            static_cast<VkInstance>(*vk_unique_instance_),
            nullptr,
            &surface))
    {
        throw std::runtime_error(std::format(
            "Error while creating Vulkan surface: {}", SDL_GetError()));
    }
    vk_surface_ = surface;
}

SDLVulkanNone::~SDLVulkanNone()
{
    if (vk_unique_instance_ && vk_surface_)
    {
        SDL_Vulkan_DestroySurface(
            static_cast<VkInstance>(*vk_unique_instance_),
            static_cast<VkSurfaceKHR>(vk_surface_),
            nullptr);
        vk_surface_ = VK_NULL_HANDLE;
    }

    vk_unique_instance_.reset();
    SDL_Vulkan_UnloadLibrary();

    if (sdl_window_)
    {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }
    SDL_Quit();
}

WindowReturnEnum SDLVulkanNone::Run(std::function<bool()> lambda)
{
    if (device_)
    {
        for (const auto& plugin_interface : device_->GetPluginPtrs())
        {
            if (plugin_interface)
            {
                plugin_interface->Startup(size_);
            }
        }
    }

    if (input_interface_)
    {
        input_interface_->NextFrame();
    }

    if (device_)
    {
        device_->Display(0.0);
        for (const auto& plugin_interface : device_->GetPluginPtrs())
        {
            if (plugin_interface)
            {
                plugin_interface->Update(*device_, 0.0);
            }
        }
    }

    if (!lambda())
    {
        return WindowReturnEnum::RESTART;
    }

    return WindowReturnEnum::UKNOWN;
}

void* SDLVulkanNone::GetGraphicContext() const
{
    return vk_unique_instance_.get();
}

vk::SurfaceKHR& SDLVulkanNone::GetVulkanSurfaceKHR()
{
    return vk_surface_;
}

} // namespace frame::vulkan
