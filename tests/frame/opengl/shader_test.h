#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/shader.h"
#include "frame/window_factory.h"

namespace test {

class ShaderTest : public testing::Test {
 public:
  ShaderTest()
      : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

 public:
  const std::string GetVertexSource() const;
  const std::string GetFragmentSource() const;

 protected:
  const glm::uvec2 size_ = {320, 200};
  std::unique_ptr<frame::WindowInterface> window_ = nullptr;
  std::unique_ptr<frame::opengl::Shader> shader_ = nullptr;
};

}  // End namespace test.
