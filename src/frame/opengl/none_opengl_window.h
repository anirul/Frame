#pragma once

#include <GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/device_interface.h"
#include "frame/draw_interface.h"
#include "frame/input_interface.h"
#include "frame/logger.h"
#include "frame/window_interface.h"

namespace frame::opengl {

class NoneOpenGLWindow : public WindowInterface {
   public:
    NoneOpenGLWindow(std::pair<std::uint32_t, std::uint32_t> size);
    virtual ~NoneOpenGLWindow();

   public:
    void Run() override;
    void* GetGraphicContext() const override;

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
    std::pair<std::uint32_t, std::uint32_t> GetDesktopSize() const override { return { 0, 0 }; }
    void* GetWindowContext() const override { return nullptr; }
    void SetWindowTitle(const std::string& title) const override {}
    WindowEnum GetWindowEnum() const override { return WindowEnum::NONE; }
    void Resize(std::pair<std::uint32_t, std::uint32_t> size) {
        size_ = size;
        device_->Resize(size);
    }
    void SetFullScreen(FullScreenEnum fullscreen_enum) {}
    FullScreenEnum GetFullScreenEnum() const { return FullScreenEnum::WINDOW; }

   private:
    std::pair<std::uint32_t, std::uint32_t> size_;
    std::unique_ptr<DeviceInterface> device_                     = nullptr;
    std::vector<std::unique_ptr<DrawInterface>> draw_interfaces_ = {};
    std::unique_ptr<InputInterface> input_interface_             = nullptr;
    frame::Logger& logger_                                       = frame::Logger::GetInstance();
#if defined(_WIN32) || defined(_WIN64)
    HWND hwnd_dummy_;  //< The dummy window.
    HDC hdc_;          //< The device context.
#endif
};

}  // End namespace frame::opengl.
