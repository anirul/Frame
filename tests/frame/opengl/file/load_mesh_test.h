#pragma once

#include <gtest/gtest.h>

#include "frame/window_factory.h"

namespace test
{

class LoadMeshTest : public testing::Test
{
  public:
    LoadMeshTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
    }

  protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.
