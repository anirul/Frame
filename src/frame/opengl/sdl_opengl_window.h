#pragma once

#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl
{

class SDLOpenGLWindow : public WindowInterface
{
  public:
    SDLOpenGLWindow(glm::uvec2 size);
    virtual ~SDLOpenGLWindow();

  public:
    void SetInputInterface(
        std::unique_ptr<InputInterface> input_interface) override
    {
        input_interface_ = std::move(input_interface);
    }
    void AddKeyCallback(std::int32_t key, std::function<bool()> func) override
    {
        key_callbacks_.insert({key, func});
    }
	void RemoveKeyCallback(std::int32_t key) override
	{
		key_callbacks_.erase(key);
	}
    void SetUniqueDevice(std::unique_ptr<DeviceInterface> device) override
    {
        device_ = std::move(device);
    }
    DeviceInterface& GetDevice() override
    {
        return *device_.get();
    }
    DrawingTargetEnum GetDrawingTargetEnum() const
    {
        return DrawingTargetEnum::WINDOW;
    }
    glm::uvec2 GetSize() const override
    {
        return size_;
    }
    glm::uvec2 GetDesktopSize() const override
    {
        return desktop_size_;
    }
    void* GetWindowContext() const override
    {
        return sdl_window_;
    }
    void SetWindowTitle(const std::string& title) const override
    {
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
    void SetOpenFileName(const std::string& file_name) override
    {
        open_file_name_ = file_name;
    }
    const std::string& GetOpenFileName() const override
    {
        return open_file_name_;
    }

  public:
    WindowReturnEnum Run(std::function<bool()> lambda) override;
    void* GetGraphicContext() const override;
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) override;
    FullScreenEnum GetFullScreenEnum() const override;
    glm::vec2 GetPixelPerInch(std::uint32_t screen = 0) const override;

  protected:
    bool RunEvent(const SDL_Event& event, const double dt);
    const char SDLButtonToChar(const Uint8 button) const;
    // Can only be called ONCE per frame!
    const double GetFrameDt(const double t) const;

  protected:
    const double GetFPS(const double dt) const
    {
        return 1.0 / dt;
    }

  private:
    glm::uvec2 size_;
    glm::uvec2 desktop_size_;
    FullScreenEnum fullscreen_enum_ = FullScreenEnum::WINDOW;
    std::unique_ptr<DeviceInterface> device_ = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    SDL_Window* sdl_window_;
    SDL_GLContext gl_context_;
    std::map<std::int32_t, std::function<bool()>> key_callbacks_ = {};
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_ = nullptr;
#endif
    std::string open_file_name_ = "";
    frame::Logger& logger_ = frame::Logger::GetInstance();
};

} // namespace frame::opengl
