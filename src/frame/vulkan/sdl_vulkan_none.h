#pragma once

#include <SDL2/SDL.h>
#include <fmt/core.h>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

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
    void Run(std::function<void()> lambda) override;
    void* GetGraphicContext() const override;

  public:
    void SetInputInterface(
        std::unique_ptr<InputInterface>&& input_interface) override
    {
        input_interface_ = std::move(input_interface);
    }
    void AddKeyCallback(std::int32_t key, std::function<bool()> func) override
    {
        throw std::runtime_error("Not implemented yet!");
    }
    void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) override
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

  private:
    glm::uvec2 size_;
    std::unique_ptr<DeviceInterface> device_ = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    SDL_Window* sdl_window_ = nullptr;
    frame::Logger& logger_ = frame::Logger::GetInstance();
    vk::raii::Context vk_context_;
    // Trick around the fact that they don't have a default constructor.
    std::optional<vk::raii::Instance> vk_instance_;
    std::optional<vk::raii::SurfaceKHR> vk_surface_KHR_;
};

} // namespace frame::vulkan.
