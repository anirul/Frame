#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#if defined(_WIN32) || defined(_WIN64)
#include <SDL2/SDL_syswm.h>
#endif
#include <fmt/core.h>

#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl {

class SDLOpenGLWindow : public WindowInterface {
   public:
    SDLOpenGLWindow(glm::uvec2 size);
    virtual ~SDLOpenGLWindow();

   public:
    void SetInputInterface(std::unique_ptr<InputInterface>&& input_interface) override {
        input_interface_ = std::move(input_interface);
    }
    void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) override {
        device_ = std::move(device);
    }
    DeviceInterface* GetUniqueDevice() override { return device_.get(); }
    glm::uvec2 GetSize() const override { return size_; }
    glm::uvec2 GetDesktopSize() const override { return desktop_size_; }
    void* GetWindowContext() const override { return sdl_window_; }
    void SetWindowTitle(const std::string& title) const override {
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }

   public:
    void Run() override;
    void* GetGraphicContext() const override;
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) override;
    FullScreenEnum GetFullScreenEnum() const override;

   protected:
    bool RunEvent(const SDL_Event& event, const double dt);
    const char SDLButtonToChar(const Uint8 button) const;
    // Can only be called ONCE per frame!
    const double GetFrameDt(const double t) const;

   protected:
    const double GetFPS(const double dt) const { return 1.0 / dt; }

   private:
    glm::uvec2 size_;
    glm::uvec2 desktop_size_;
    FullScreenEnum fullscreen_enum_                  = FullScreenEnum::WINDOW;
    std::unique_ptr<DeviceInterface> device_         = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    SDL_Window* sdl_window_                          = nullptr;
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_ = nullptr;
#endif
    frame::Logger& logger_ = frame::Logger::GetInstance();
};

}  // namespace frame::opengl
