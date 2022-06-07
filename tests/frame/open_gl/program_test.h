#pragma once

#include <gtest/gtest.h>

#include "frame/open_gl/program.h"
#include "frame/window.h"

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
