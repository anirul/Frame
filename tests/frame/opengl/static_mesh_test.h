#pragma once

#include <gtest/gtest.h>

#include "frame/static_mesh_interface.h"
#include "frame/window_factory.h"

namespace test {

class StaticMeshTest : public testing::Test {
 public:
  StaticMeshTest()
      : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

 protected:
  const glm::uvec2 size_ = {320, 200};
  std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
