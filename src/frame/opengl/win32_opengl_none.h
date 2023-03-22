#pragma once

#include <GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/device_interface.h"
#include "frame/input_interface.h"
#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl {

class Win32OpenGLNone : public WindowInterface {
   public:
    Win32OpenGLNone(glm::uvec2 size);
    virtual ~Win32OpenGLNone();

   public:
    void Run(std::function<void()> lambda) override;
    void* GetGraphicContext() const override;

   public:
    void SetInputInterface(std::unique_ptr<InputInterface>&& input_interface) override {
        input_interface_ = std::move(input_interface);
    }
	void AddKeyCallback(std::int32_t key, std::function<bool()> func) override {
		throw std::runtime_error("Not implemented.");
	}
    void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) override {
        device_ = std::move(device);
    }
    DeviceInterface& GetUniqueDevice() override { return *device_.get(); }
    glm::uvec2 GetSize() const override { return size_; }
    glm::uvec2 GetDesktopSize() const override { return { 0, 0 }; }
    void* GetWindowContext() const override { return nullptr; }
    void SetWindowTitle(const std::string& title) const override {}
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum) {
        size_ = size;
        device_->Resize(size);
    }
    FullScreenEnum GetFullScreenEnum() const { return FullScreenEnum::WINDOW; }
    glm::vec2 GetPixelPerInch(std::uint32_t screen = 0) const override {
        throw std::runtime_error("This is a none device so no screen.");
    }

   private:
    glm::uvec2 size_;
    std::unique_ptr<DeviceInterface> device_         = nullptr;
    std::unique_ptr<InputInterface> input_interface_ = nullptr;
    frame::Logger& logger_                           = frame::Logger::GetInstance();
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_dummy_;  //< The dummy window.
    HDC hdc_;          //< The device context.
#endif
};

}  // End namespace frame::opengl.
