#pragma once

#include <gtest/gtest.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/open_gl/device.h"
#include "frame/json/parse_level.h"
#include "frame/json/proto.h"
#include "frame/window.h"

namespace test {

class DeviceTest : public ::testing::Test {
   public:
    DeviceTest() : window_(frame::CreateSDLOpenGL(size_)) {
        auto maybe_level = frame::proto::ParseLevelOpenGL(
            size_, frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
                       frame::file::FindFile("asset/json/device_test.json")));
        if (!maybe_level) throw std::runtime_error("Couldn't create level.");
        level_ = std::move(maybe_level.value());
    }

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::LevelInterface> level_       = nullptr;
};

}  // End namespace test.
