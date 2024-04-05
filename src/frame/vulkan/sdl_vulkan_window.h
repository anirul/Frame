#pragma once

#include <SDL2/SDL.h>
// Some windows declarations.
#if defined(_WIN32) || defined(_WIN64)
#include <SDL2/SDL_syswm.h>
#endif
#include <vulkan/vulkan_raii.hpp>
#include <fmt/core.h>
#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"
#include "frame/vulkan/vulkan_window_interface.h"
#include "frame/vulkan/device.h"

namespace frame::vulkan
{

class SDLVulkanWindow : public VulkanWindowInterface
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
    void Run(std::function<void()> lambda = [] {}) override;
    void* GetGraphicContext() const override;
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) override;
    FullScreenEnum GetFullScreenEnum() const override;
    vk::InstanceCreateInfo GetInstanceCreateInfo(
        vk::ApplicationInfo app_info = {
            "Frame Vulkan",
            VK_MAKE_VERSION(1, 0, 0),
            "Frame (SDL Vulkan Window)",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_3}) const override;
    void SetUniqueDevice(std::unique_ptr<DeviceInterface> device) override;

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
    std::optional<vk::raii::SurfaceKHR> surface_khr_;
};

} // namespace frame::vulkan.
