#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/shader.h"
#include "frame/opengl/window.h"

namespace test {

class ShaderTest : public testing::Test {
   public:
    ShaderTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   public:
    const std::string GetVertexSource() const;
    const std::string GetFragmentSource() const;

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::opengl::Shader> shader_      = nullptr;
};

}  // End namespace test.
