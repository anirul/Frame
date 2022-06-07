#pragma once

#include <filesystem>

#include "frame/device_interface.h"
#include "frame/draw_interface.h"
#include "frame/logger.h"
#include "frame/window.h"
#include "examples/lib_common/path_interface.h"

class Draw : public frame::DrawInterface {
   public:
    Draw(const std::pair<std::uint32_t, std::uint32_t> size, std::filesystem::path path,
         frame::DeviceInterface* device)
        : size_(size), path_(path), device_(device) {}

   public:
    void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
    void RunDraw(double dt) override;

   private:
    frame::Logger& logger_                        = frame::Logger::GetInstance();
    std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
    std::filesystem::path path_;
    frame::DeviceInterface* device_ = nullptr;
};
