#pragma once

#include <gtest/gtest.h>

#include "Frame/OpenGL/Program.h"
#include "Frame/Window.h"

namespace test {

class ProgramTest : public testing::Test {
   public:
    ProgramTest() : window_(frame::CreateSDLOpenGL(size_)) {}

   public:
    const std::string GetVertexSource() const;
    const std::string GetFragmentSource() const;

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_   = nullptr;
};

}  // End namespace test.
