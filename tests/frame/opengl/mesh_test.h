#pragma once

#include <gtest/gtest.h>

#include "frame/mesh_interface.h"
#include "frame/window_factory.h"

namespace test
{

class MeshTest : public testing::Test
{
  public:
    MeshTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
    }

  protected:
    const glm::uvec2 size_ = {320, 200};
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.

