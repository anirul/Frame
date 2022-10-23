#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <fmt/core.h>

#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl {

class SDLOpenGLWindow : public WindowInterface {
   public:
    SDLOpenGLWindow(std::pair<std::uint32_t, std::uint32_t> size);
    virtual ~SDLOpenGLWindow();

   public:
    int AddDrawInterface(std::unique_ptr<DrawInterface>&& draw_interface) override {
        int index = static_cast<int>(draw_interfaces_.size());
        draw_interfaces_.push_back(std::move(draw_interface));
        return index;
    }
    void RemoveDrawInterface(int index) override { draw_interfaces_[index] = nullptr; } 
    void SetInputInterface(std::unique_ptr<InputInterface>&& input_interface) override {
        input_interface_ = std::move(input_interface);
    }
    void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) override {
        device_ = std::move(device);
    }
    DeviceInterface* GetUniqueDevice() override { return device_.get(); }
    std::pair<std::uint32_t, std::uint32_t> GetSize() const override { return size_; }
    std::pair<std::uint32_t, std::uint32_t> GetDesktopSize() const override {
        return desktop_size_;
    }
    void* GetWindowContext() const override { return sdl_window_; }
    void SetWindowTitle(const std::string& title) const override {
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
    WindowEnum GetWindowEnum() const override { return WindowEnum::SDL2; }

   public:
    void Run() override;
    void* GetGraphicContext() const override;
    void Resize(std::pair<std::uint32_t, std::uint32_t> size) override;
    void SetFullScreen(FullScreenEnum fullscreen_enum) override;
    FullScreenEnum GetFullScreenEnum() const override;

   protected:
    bool RunEvent(const SDL_Event& event, const double dt);
    const char SDLButtonToChar(const Uint8 button) const;
    // Can only be called ONCE per frame!
    const double GetFrameDt(const double t) const;

   protected:
    const double GetFPS(const double dt) const { return 1.0 / dt; }

   private:
    std::pair<std::uint32_t, std::uint32_t> size_;
    std::pair<std::uint32_t, std::uint32_t> desktop_size_;
    FullScreenEnum fullscreen_enum_                              = FullScreenEnum::WINDOW;
    std::unique_ptr<DeviceInterface> device_                     = nullptr;
    std::vector<std::unique_ptr<DrawInterface>> draw_interfaces_ = {};
    std::unique_ptr<InputInterface> input_interface_             = nullptr;
    SDL_Window* sdl_window_                                      = nullptr;
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_ = nullptr;
#endif
    frame::Logger& logger_ = frame::Logger::GetInstance();
};

}  // namespace frame::opengl
