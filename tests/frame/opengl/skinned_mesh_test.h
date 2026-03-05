#pragma once

#include <gtest/gtest.h>

#include "frame/window_factory.h"

namespace test
{

class SkinnedMeshTest : public testing::Test
{
  public:
    SkinnedMeshTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
    }

  protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.

