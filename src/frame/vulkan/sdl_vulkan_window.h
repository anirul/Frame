#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#endif
#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::vulkan
{

class SDLVulkanWindow : public WindowInterface
{
  public:
    SDLVulkanWindow(glm::uvec2 size);
    virtual ~SDLVulkanWindow();

  public:
    void SetInputInterface(
        std::unique_ptr<InputInterface> input_interface) override
    {
        input_interface_ = std::move(input_interface);
    }
    void AddKeyCallback(std::int32_t key, std::function<bool()> func) override
    {
        throw std::runtime_error("Not implemented yet!");
    }
	void RemoveKeyCallback(std::int32_t key) override
	{
		throw std::runtime_error("Not implemented yet!");
	}
    void SetUniqueDevice(std::unique_ptr<DeviceInterface> device) override
    {
        device_ = std::move(device);
    }
    DeviceInterface& GetDevice() override
    {
        return *device_.get();
    }
    glm::uvec2 GetSize() const override
    {
        return size_;
    }
    glm::vec2 GetPixelPerInch(std::uint32_t screen = 0) const
    {
        throw std::runtime_error("Not implemented yet!");
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
    DrawingTargetEnum GetDrawingTargetEnum() const override
    {
        return DrawingTargetEnum::WINDOW;
    }

  public:
    WindowReturnEnum Run(
		std::function<bool()> lambda = [] { return true; }) override;
    void* GetGraphicContext() const override;
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) override;
    FullScreenEnum GetFullScreenEnum() const override;

  public:
    vk::SurfaceKHR& GetVulkanSurfaceKHR()
    {
        return vk_surface_.get();
    }

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
    SDL_Window* sdl_window_ = nullptr;
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_ = nullptr;
#endif
    frame::Logger& logger_ = frame::Logger::GetInstance();
    vk::UniqueInstance vk_unique_instance_;
    vk::UniqueSurfaceKHR vk_surface_;
};

} // namespace frame::vulkan.
