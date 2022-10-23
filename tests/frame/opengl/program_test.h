#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/program.h"
#include "frame/opengl/window.h"

namespace test {

class ProgramTest : public testing::Test {
   public:
    ProgramTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   public:
    const std::string GetVertexSource() const;
    const std::string GetFragmentSource() const;

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_   = nullptr;
};

}  // End namespace test.
