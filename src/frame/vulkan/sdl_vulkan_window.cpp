#include "frame/vulkan/sdl_vulkan_window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#include "frame/gui/draw_gui_interface.h"
#include "frame/vulkan/debug_callback.h"

namespace frame::vulkan {

SDLVulkanWindow::SDLVulkanWindow(glm::uvec2 size) : size_(size) {
    // Initialize SDL with the video subsystem.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(fmt::format("Couldn't initialize SDL: {}", SDL_GetError()));
    }

    if (SDL_Vulkan_LoadLibrary(nullptr) == -1) {
        throw std::runtime_error(fmt::format("Couldn't load Vulkan library: {}", SDL_GetError()));
    }

    // Create an SDL window to use as a surface for Vulkan.
    sdl_window_ = SDL_CreateWindow("Vulkan Headless", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, size_.x, size_.y, SDL_WINDOW_VULKAN);
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
    extensions.push_back("VK_EXT_debug_utils");
    for (const auto& extension : extensions) {
        logger_->info("Extension: {}", extension);
    }

    vk::ApplicationInfo application_info("Frame", VK_MAKE_VERSION(0, 5, 1), "SDL - Vulkan - Window",
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

SDLVulkanWindow::~SDLVulkanWindow() {
    // Destroy the surface and instance when finished
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLVulkanWindow::Run() {
    // Called only once at the beginning.
    for (const auto& plugin_interface : device_->GetPluginPtrs()) {
        // In case this is a removed one it will be nulled.
        if (plugin_interface) {
            // This will call the device startup.
            plugin_interface->Startup(size_);
        }
    }
    // While Run return true continue.
    bool loop = true;
    // Timing counter.
    auto start = std::chrono::system_clock::now();
    do {
        // Compute the time difference from previous frame.
        auto end                           = std::chrono::system_clock::now();
        std::chrono::duration<double> time = end - start;
        const double dt                    = GetFrameDt(time.count());

        // Process events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            bool skip = false;
            for (const auto& plugin_interface : device_->GetPluginPtrs()) {
                if (plugin_interface) {
                    if (plugin_interface->PollEvent(&event)) skip = true;
                }
            }
            if (skip) continue;
            if (!RunEvent(event, dt)) {
                loop = false;
            }
        }
        if (input_interface_) input_interface_->NextFrame();

        device_->Display(time.count());

        // Draw the Scene not used?
        for (const auto& plugin_interface : device_->GetPluginPtrs()) {
            if (plugin_interface) {
                if (!plugin_interface->Update(device_.get(), time.count())) {
                    loop = false;
                }
            }
        }

        SetWindowTitle("SDL Vulkan - " + std::to_string(static_cast<float>(GetFPS(dt))));

        // Swap the window.
        throw std::runtime_error("Implement swap window.");
    } while (loop);
}

void* SDLVulkanWindow::GetGraphicContext() const { return vk_unique_instance_.get(); }

void SDLVulkanWindow::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) {
    size_ = size;
    if (fullscreen_enum_ != fullscreen_enum) {
        fullscreen_enum_      = fullscreen_enum;
        SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(0);
        if (fullscreen_enum_ == FullScreenEnum::WINDOW) {
            flags = static_cast<SDL_WindowFlags>(0);
        }
        if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN) {
            flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN;
        }
        if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN_DESKTOP) {
            flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
        // Change window mode.
        if (SDL_SetWindowFullscreen(sdl_window_, flags) < 0) {
            throw std::runtime_error(
                fmt::format("Error switching to fullscreen mode: {}", SDL_GetError()));
        }
        // Only resize when changing window mode.
        SDL_SetWindowSize(sdl_window_, size_.x, size_.y);
    }
    device_->Resize(size_);
}

frame::FullScreenEnum SDLVulkanWindow::GetFullScreenEnum() const { return fullscreen_enum_; }

bool SDLVulkanWindow::RunEvent(const SDL_Event& event, const double dt) {
    if (event.type == SDL_QUIT) return false;
    bool has_window_plugin = false;
    for (PluginInterface* plugin : device_->GetPluginPtrs()) {
        if (dynamic_cast<frame::gui::DrawGuiInterface*>(plugin)) has_window_plugin = true;
    }
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE: {
                if (has_window_plugin) {
                    for (PluginInterface* plugin : device_->GetPluginPtrs()) {
                        auto* window_plugin = dynamic_cast<frame::gui::DrawGuiInterface*>(plugin);
                        if (window_plugin) {
                            auto is_visible = window_plugin->IsVisible();
                            window_plugin->SetVisible(!is_visible);
                        }
                    }
                    return true;
                }
                return false;
            }
            case SDLK_PRINTSCREEN:
                device_->ScreenShot("ScreenShot.png");
                return true;
        }
    }
    if (input_interface_) {
        if (event.type == SDL_KEYDOWN) {
            return input_interface_->KeyPressed(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_KEYUP) {
            return input_interface_->KeyReleased(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_MOUSEMOTION) {
            return input_interface_->MouseMoved(glm::vec2(event.motion.x, event.motion.y),
                                                glm::vec2(event.motion.xrel, event.motion.yrel),
                                                dt);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            return input_interface_->MousePressed(SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            return input_interface_->MouseReleased(SDLButtonToChar(event.button.button), dt);
        }

        if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0) {
            return input_interface_->WheelMoved(static_cast<float>(event.wheel.y), dt);
        }
    }
    return true;
}

const char SDLVulkanWindow::SDLButtonToChar(const Uint8 button) const {
    char ret = 0;
    if (button & SDL_BUTTON_LEFT) ret += 1;
    if (button & SDL_BUTTON_RIGHT) ret += 2;
    if (button & SDL_BUTTON_MIDDLE) ret += 4;
    if (button & SDL_BUTTON_X1) ret += 8;
    if (button & SDL_BUTTON_X2) ret += 16;
    return ret;
}

const double SDLVulkanWindow::GetFrameDt(const double t) const {
    static double previous_t = 0.0;
    double ret               = t - previous_t;
    previous_t               = t;
    return ret;
}

}  // End namespace frame::vulkan.
