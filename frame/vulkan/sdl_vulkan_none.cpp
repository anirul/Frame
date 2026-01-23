#include "frame/vulkan/sdl_vulkan_none.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstring>
#include <format>
#include <string>
#include <vector>

#include "absl/flags/flag.h"

#include "frame/common/application.h"
#include "frame/vulkan/debug_callback.h"

namespace frame::vulkan
{
namespace
{
constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

bool HasExtension(
    const std::vector<vk::ExtensionProperties>& extensions,
    const char* name)
{
    for (const auto& ext : extensions)
    {
        if (std::strcmp(ext.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

bool HasLayer(
    const std::vector<vk::LayerProperties>& layers,
    const char* name)
{
    for (const auto& layer : layers)
    {
        if (std::strcmp(layer.layerName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

bool IsValidationEnabled()
{
#if defined(_DEBUG)
    bool enabled = true;
#else
    bool enabled = false;
#endif
    enabled = absl::GetFlag(FLAGS_vk_validation);
    return enabled;
}
} // namespace

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
    const auto available_extensions =
        vk::enumerateInstanceExtensionProperties();
    const bool want_validation = IsValidationEnabled();
    const bool has_debug_utils =
        HasExtension(available_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (want_validation && has_debug_utils)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    for (const auto& extension : extensions)
    {
        logger_->info("Extension: {}", extension);
    }

    std::vector<const char*> layers;
    if (want_validation)
    {
        const auto available_layers = vk::enumerateInstanceLayerProperties();
        if (HasLayer(available_layers, kValidationLayerName))
        {
            layers.push_back(kValidationLayerName);
        }
        else
        {
            logger_->warn("Vulkan validation layer not found.");
        }
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
        static_cast<std::uint32_t>(layers.size()),
        layers.data(),
        static_cast<std::uint32_t>(extensions.size()),
        extensions.data());

    vk::DebugUtilsMessengerCreateInfoEXT debug_info{};
    if (want_validation && has_debug_utils)
    {
        debug_info.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debug_info.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debug_info.pfnUserCallback =
            reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(
                DebugCallback);
        instance_create_info.setPNext(&debug_info);
    }

    vk_unique_instance_ = vk::createInstanceUnique(instance_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*vk_unique_instance_, vk_get_instance_proc_addr);
    if (want_validation && has_debug_utils)
    {
        debug_messenger_ =
            vk_unique_instance_->createDebugUtilsMessengerEXTUnique(debug_info);
    }

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
    device_.reset();

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
        try
        {
            device_->Display(0.0);
        }
        catch (const std::exception& ex)
        {
            logger_->error("Vulkan Display failed: {}", ex.what());
            return WindowReturnEnum::QUIT;
        }
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
