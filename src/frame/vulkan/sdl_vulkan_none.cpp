#include "frame/vulkan/sdl_vulkan_none.h"

#include "frame/vulkan/debug_callback.h"
#include "frame/vulkan/device.h"
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
}

vk::InstanceCreateInfo SDLVulkanNone::GetInstanceCreateInfo(
    vk::ApplicationInfo app_info) const
{
    static std::vector<const char*> sdl_extensions;
    unsigned int sdl_extension_count = 0;
    SDL_Vulkan_GetInstanceExtensions(
        sdl_window_, &sdl_extension_count, nullptr);
    sdl_extensions.resize(sdl_extension_count);
    SDL_Vulkan_GetInstanceExtensions(
        sdl_window_, &sdl_extension_count, sdl_extensions.data());
#ifdef _DEBUG
    // Enable the debug callback extension.
    sdl_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    static std::vector<const char*> layers;
#ifdef _DEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    return vk::InstanceCreateInfo(
        {},
        &app_info,
        static_cast<uint32_t>(layers.size()),
        layers.data(),
        static_cast<uint32_t>(sdl_extensions.size()),
        sdl_extensions.data());
}

void SDLVulkanNone::SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device)
{
    device_ = std::move(device);
    vulkan::Device* vulkan_device =
        dynamic_cast<vulkan::Device*>(device_.get());
    if (!vulkan_device)
    {
        throw std::runtime_error("Device is not a vulkan device.");
    }

    vulkan_device->Init(GetInstanceCreateInfo());
    VkSurfaceKHR c_surface;
    vk::raii::Instance instance = vulkan_device->MoveInstance();
    if (!SDL_Vulkan_CreateSurface(
            sdl_window_, *instance, &c_surface))
    {
        throw std::runtime_error(
            fmt::format("Couldn't create surface: {}", SDL_GetError()));
    }
    surface_khr_.emplace(instance, c_surface);
    vulkan_device->EmplaceInstance(std::move(instance));
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
    return sdl_window_;
}

} // End namespace frame::vulkan.
