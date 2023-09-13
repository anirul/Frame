#pragma once

#include <gtest/gtest.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/json/proto.h"
#include "frame/opengl/device.h"
#include "frame/window_factory.h"

namespace test
{

class DeviceTest : public ::testing::Test
{
  public:
    DeviceTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
        auto level = frame::proto::ParseLevel(
            size_, frame::file::FindFile("asset/json/device_test.json"));
        if (!level)
            throw std::runtime_error("Couldn't create level.");
        level_ = std::move(level);
    }

  protected:
    const glm::uvec2 size_ = {320, 200};
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
};

} // End namespace test.
