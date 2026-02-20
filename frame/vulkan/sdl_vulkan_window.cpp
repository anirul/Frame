#include "frame/vulkan/sdl_vulkan_window.h"

// Needed under Windows to compute DPI.
#if defined(_WIN32) || defined(_WIN64)
#include <shellscalingapi.h>
#include <shtypes.h>
#pragma comment(lib, "Shcore.lib")
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <chrono>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

#include "absl/flags/flag.h"

#include "frame/common/application.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/vulkan/debug_callback.h"

namespace frame::vulkan
{
namespace
{
constexpr const char* kDefaultTitle = "Frame Vulkan";
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

#if defined(_WIN32) || defined(_WIN64)
std::vector<glm::vec2> g_monitor_ppi;

BOOL MonitorEnumProc(HMONITOR hmonitor, HDC, LPRECT, LPARAM)
{
    std::uint32_t hppi = 0;
    std::uint32_t vppi = 0;
    if (GetDpiForMonitor(hmonitor, MDT_RAW_DPI, &hppi, &vppi) != S_OK)
    {
        throw std::runtime_error("Couldn't get the PPI.");
    }

    MONITORINFOEX monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfo(hmonitor, &monitor_info))
    {
        throw std::runtime_error("Couldn't get monitor info.");
    }
    const int cx_logical =
        monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    const int cy_logical =
        monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

    DEVMODE display_mode{};
    display_mode.dmSize = sizeof(display_mode);
    display_mode.dmDriverExtra = 0;
    if (!EnumDisplaySettings(
            monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &display_mode))
    {
        throw std::runtime_error("Couldn't enumerate display settings.");
    }
    const int cx_physical = display_mode.dmPelsWidth;
    const int cy_physical = display_mode.dmPelsHeight;

    const float horz_scale =
        static_cast<float>(cx_physical) / static_cast<float>(cx_logical);
    const float vert_scale =
        static_cast<float>(cy_physical) / static_cast<float>(cy_logical);
    const float uniform_scale = (horz_scale + vert_scale) * 0.5f;

    g_monitor_ppi.emplace_back(
        static_cast<float>(hppi) * uniform_scale,
        static_cast<float>(vppi) * uniform_scale);

    return TRUE;
}
#endif
}

void SDLVulkanWindow::AddKeyCallback(
    std::int32_t key,
    std::function<bool()> func)
{
    key_callbacks_[key] = std::move(func);
}

void SDLVulkanWindow::RemoveKeyCallback(std::int32_t key)
{
    key_callbacks_.erase(key);
}

SDLVulkanWindow::SDLVulkanWindow(glm::uvec2 size) : size_(size)
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

    const Uint32 window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    sdl_window_ =
        SDL_CreateWindow(kDefaultTitle, size_.x, size_.y, window_flags);
    if (!sdl_window_)
    {
        throw std::runtime_error(
            std::format("Couldn't initialize window: {}", SDL_GetError()));
    }

    SDL_DisplayID display_id = SDL_GetDisplayForWindow(sdl_window_);
    SDL_Rect bounds{};
    if (display_id && SDL_GetDisplayBounds(display_id, &bounds))
    {
        desktop_size_ = glm::uvec2(bounds.w, bounds.h);
    }
    else
    {
        desktop_size_ = size_;
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
        "SDL - Vulkan - Window",
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

#if defined(_WIN32) || defined(_WIN64)
    hwnd_ = FindWindowA(nullptr, SDL_GetWindowTitle(sdl_window_));
#endif
}

SDLVulkanWindow::~SDLVulkanWindow()
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

    // Destroy instance-scoped debug messenger before destroying the instance.
    debug_messenger_.reset();
    vk_unique_instance_.reset();
    SDL_Vulkan_UnloadLibrary();

    if (sdl_window_)
    {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }
    SDL_Quit();
}

WindowReturnEnum SDLVulkanWindow::Run(std::function<bool()> lambda)
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

    WindowReturnEnum window_return_enum = WindowReturnEnum::CONTINUE;
    auto start = std::chrono::steady_clock::now();

    while (window_return_enum == WindowReturnEnum::CONTINUE)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        const double dt = GetFrameDt(elapsed.count());

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            bool consumed = false;
            if (device_)
            {
                for (const auto& plugin_interface : device_->GetPluginPtrs())
                {
                    if (plugin_interface && plugin_interface->PollEvent(&event))
                    {
                        consumed = true;
                    }
                }
            }

            if (!consumed && !RunEvent(event, dt))
            {
                window_return_enum = WindowReturnEnum::QUIT;
            }
        }

        if (window_return_enum != WindowReturnEnum::CONTINUE)
        {
            break;
        }

        if (input_interface_)
        {
            input_interface_->NextFrame();
        }

        if (device_)
        {
            try
            {
                device_->Display(dt);
            }
            catch (const std::exception& ex)
            {
                logger_->error("Vulkan Display failed: {}", ex.what());
                window_return_enum = WindowReturnEnum::QUIT;
            }

            for (const auto& plugin_interface : device_->GetPluginPtrs())
            {
                if (plugin_interface &&
                    !plugin_interface->Update(*device_, elapsed.count()))
                {
                    window_return_enum = WindowReturnEnum::RESTART;
                }
            }
        }

        std::string title = "Frame - Vulkan";
        if (!open_file_name_.empty())
        {
            title += " - " + open_file_name_;
        }
        title += std::format(" - {:.2f}", GetFPS(dt));
        SetWindowTitle(title);

        if (!lambda())
        {
            window_return_enum = WindowReturnEnum::RESTART;
        }

        SDL_Delay(1);
    }

    return window_return_enum;
}

void* SDLVulkanWindow::GetGraphicContext() const
{
    return vk_unique_instance_.get();
}

void SDLVulkanWindow::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum)
{
    size_ = size;

    if (fullscreen_enum_ != fullscreen_enum)
    {
        fullscreen_enum_ = fullscreen_enum;

        const SDL_DisplayMode* mode_ptr = nullptr;
        SDL_DisplayMode fullscreen_mode{};

        if (fullscreen_enum_ == FullScreenEnum::WINDOW)
        {
            mode_ptr = nullptr;
        }
        else if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN)
        {
            SDL_DisplayID display = SDL_GetDisplayForWindow(sdl_window_);
            if (!display)
            {
                throw std::runtime_error(std::format(
                    "SDL_GetDisplayForWindow failed: {}", SDL_GetError()));
            }

            SDL_Rect bounds;
            if (!SDL_GetDisplayBounds(display, &bounds))
            {
                throw std::runtime_error(std::format(
                    "SDL_GetDisplayBounds failed: {}", SDL_GetError()));
            }

            fullscreen_mode.w = bounds.w;
            fullscreen_mode.h = bounds.h;
            mode_ptr = &fullscreen_mode;
        }
        else if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN_DESKTOP)
        {
            fullscreen_mode.w = 0;
            fullscreen_mode.h = 0;
            fullscreen_mode.refresh_rate = 0;
            mode_ptr = &fullscreen_mode;
        }

        if (!SDL_SetWindowFullscreenMode(sdl_window_, mode_ptr))
        {
            throw std::runtime_error(std::format(
                "Error switching fullscreen mode: {}", SDL_GetError()));
        }

        if (fullscreen_enum_ == FullScreenEnum::WINDOW)
        {
            SDL_SetWindowSize(sdl_window_, size_.x, size_.y);
        }
    }

    if (device_)
    {
        device_->Resize(size_);
    }
}

frame::FullScreenEnum SDLVulkanWindow::GetFullScreenEnum() const
{
    return fullscreen_enum_;
}

glm::vec2 SDLVulkanWindow::GetPixelPerInch(std::uint32_t screen) const
{
#if defined(_WIN32) || defined(_WIN64)
    g_monitor_ppi.clear();
    if (!EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0))
    {
        throw std::runtime_error("Couldn't enumerate monitors.");
    }
    if (screen >= g_monitor_ppi.size())
    {
        throw std::runtime_error("Outside screen.");
    }
    return g_monitor_ppi[screen];
#else
    int display_count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&display_count);
    SDL_DisplayID target_display = SDL_GetPrimaryDisplay();
    if (displays)
    {
        if (screen < static_cast<std::uint32_t>(display_count))
        {
            target_display = displays[screen];
        }
        SDL_free(displays);
    }
    const float scale = SDL_GetDisplayContentScale(target_display);
    if (scale <= 0.0f)
    {
        throw std::runtime_error("Error couldn't get the DPI");
    }
    const float dpi = scale * 96.0f;
    return glm::vec2(dpi, dpi);
#endif
}

bool SDLVulkanWindow::RunEvent(const SDL_Event& event, const double dt)
{
    if (event.type == SDL_EVENT_QUIT)
    {
        return false;
    }

    if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        SDL_StartTextInput(sdl_window_);
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        SDL_StopTextInput(sdl_window_);
    }

    bool has_window_plugin = false;
    if (device_)
    {
        for (PluginInterface* plugin : device_->GetPluginPtrs())
        {
            if (dynamic_cast<frame::gui::DrawGuiInterface*>(plugin))
            {
                has_window_plugin = true;
            }
        }
    }

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        switch (event.key.key)
        {
        case SDLK_ESCAPE:
            if (has_window_plugin)
            {
                for (PluginInterface* plugin : device_->GetPluginPtrs())
                {
                    auto* window_plugin =
                        dynamic_cast<frame::gui::DrawGuiInterface*>(plugin);
                    if (window_plugin)
                    {
                        const bool is_visible = window_plugin->IsVisible();
                        window_plugin->SetVisible(!is_visible);
                    }
                }
                return true;
            }
            return false;
        case SDLK_PRINTSCREEN:
            if (device_)
            {
                device_->ScreenShot("ScreenShot.png");
            }
            return true;
        default:
            if (key_callbacks_.count(event.key.key))
            {
                return key_callbacks_.at(event.key.key)();
            }
            break;
        }
    }

    if (input_interface_)
    {
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            return input_interface_->KeyPressed(event.key.key, dt);
        }
        if (event.type == SDL_EVENT_KEY_UP)
        {
            return input_interface_->KeyReleased(event.key.key, dt);
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            return input_interface_->MouseMoved(
                glm::vec2(event.motion.x, event.motion.y),
                glm::vec2(event.motion.xrel, event.motion.yrel),
                dt);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            return input_interface_->MousePressed(
                SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            return input_interface_->MouseReleased(
                SDLButtonToChar(event.button.button), dt);
        }

        if (event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.y != 0)
        {
            return input_interface_->WheelMoved(
                static_cast<float>(event.wheel.y), dt);
        }
    }

    return true;
}

const char SDLVulkanWindow::SDLButtonToChar(const Uint8 button) const
{
    char ret = 0;
    if (button & SDL_BUTTON_LEFT)
        ret += 1;
    if (button & SDL_BUTTON_RIGHT)
        ret += 2;
    if (button & SDL_BUTTON_MIDDLE)
        ret += 4;
    if (button & SDL_BUTTON_X1)
        ret += 8;
    if (button & SDL_BUTTON_X2)
        ret += 16;
    return ret;
}

double SDLVulkanWindow::GetFrameDt(double t) const
{
    static double previous_t = 0.0;
    const double ret = t - previous_t;
    previous_t = t;
    return ret;
}

vk::SurfaceKHR& SDLVulkanWindow::GetVulkanSurfaceKHR()
{
    return vk_surface_;
}

} // namespace frame::vulkan
