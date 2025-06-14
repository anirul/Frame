#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_interface.h"
#include "frame/window_factory.h"

namespace test
{

class ImageBasedLightingTest : public testing::Test
{
  public:
    ImageBasedLightingTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
        auto level = frame::json::ParseLevel(
            size_,
            frame::file::FindFile("asset/json/image_based_lighting.json"));
        if (!level)
            throw std::runtime_error("Couldn't parse level.");
        level_ = std::move(level);
    }

  protected:
    const glm::uvec2 size_{320, 200};
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
};

} // namespace test
