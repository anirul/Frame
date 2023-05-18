#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"
#include "frame/window_factory.h"

namespace test {

class LoadStaticMeshTest : public testing::Test {
 public:
  LoadStaticMeshTest()
      : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

 protected:
  std::shared_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
