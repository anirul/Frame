#pragma once

#include <gtest/gtest.h>

#include "frame/level_interface.h"
#include "frame/window_factory.h"

namespace test
{

class ParseLevelTest : public testing::Test
{
  public:
    ParseLevelTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
    }

  protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
    std::shared_ptr<frame::LevelInterface> level_ = nullptr;
};

} // End namespace test.
