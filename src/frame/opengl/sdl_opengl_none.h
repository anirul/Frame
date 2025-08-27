#pragma once

#define NOMINMAX
#include <SDL3/SDL.h>
#include <format>
#include <glad/glad.h>
#include <string>

#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl
{

class SDLOpenGLNone : public WindowInterface
{
  public:
    SDLOpenGLNone(glm::uvec2 size);
    virtual ~SDLOpenGLNone();

  public:
    WindowReturnEnum Run(std::function<bool()> lambda) override;
    void* GetGraphicContext() const override;

  public:
    void SetInputInterface(
        std::unique_ptr<InputInterface> input_interface) override
    {
        input_interface_ = std::move(input_interface);
    }
    void AddKeyCallback(std::int32_t key, std::function<bool()> func) override
    {
        throw std::runtime_error("Not implemented.");
    }
    void RemoveKeyCallback(std::int32_t key) override
    {
        throw std::runtime_error("Not implemented.");
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
        return DrawingTargetEnum::NONE;
    }
    glm::uvec2 GetSize() const override
    {
        return size_;
    }
    glm::uvec2 GetDesktopSize() const override
    {
        return {0, 0};
    }
    void* GetWindowContext() const override
    {
        return sdl_window_;
    }
    void SetWindowTitle(const std::string& title) const override
    {
    }
    void SetOpenFileName(const std::string& file_name) override
    {
        open_file_name_ = file_name;
    }
    const std::string& GetOpenFileName() const override
    {
        return open_file_name_;
    }
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) override
    {
        size_ = size;
        device_->Resize(size);
    }
    FullScreenEnum GetFullScreenEnum() const override
    {
        return FullScreenEnum::WINDOW;
    }
    glm::vec2 GetPixelPerInch(std::uint32_t screen = 0) const override
    {
        throw std::runtime_error("This is a none device so no screen.");
    }

  private:
    glm::uvec2 size_;
    std::unique_ptr<DeviceInterface> device_ = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    SDL_Window* sdl_window_ = nullptr;
    SDL_GLContext gl_context_;
    std::string open_file_name_ = "";
    frame::Logger& logger_ = frame::Logger::GetInstance();
};

} // namespace frame::opengl
