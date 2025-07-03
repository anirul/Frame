#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <stdexcept>

#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::vulkan
{

class SDLVulkanNone : public WindowInterface
{
  public:
    SDLVulkanNone(glm::uvec2 size);
    virtual ~SDLVulkanNone();

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
    DrawingTargetEnum GetDrawingTargetEnum() const override
    {
        return DrawingTargetEnum::NONE;
    }

  public:
    vk::SurfaceKHR& GetVulkanSurfaceKHR()
    {
        return vk_surface_.get();
    }

  private:
    glm::uvec2 size_;
    std::unique_ptr<DeviceInterface> device_ = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    SDL_Window* sdl_window_ = nullptr;
    std::string open_file_name_ = "";
    frame::Logger& logger_ = frame::Logger::GetInstance();
    vk::UniqueInstance vk_unique_instance_;
    vk::UniqueSurfaceKHR vk_surface_;
};

} // namespace frame::vulkan.
