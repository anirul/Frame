#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/frame_buffer.h"
#include "frame/window_factory.h"

namespace test {

class FrameBufferTest : public testing::Test {
 public:
  FrameBufferTest()
      : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

 protected:
  const glm::uvec2 size_ = {320, 200};
  std::unique_ptr<frame::opengl::FrameBuffer> frame_ = nullptr;
  std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
