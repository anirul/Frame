#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/program.h"
#include "frame/window_factory.h"

namespace test {

class ProgramTest : public testing::Test {
   public:
    ProgramTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

   public:
    const std::string GetVertexSource() const;
    const std::string GetFragmentSource() const;

   protected:
    const glm::uvec2 size_                            = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_   = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
};

}  // End namespace test.
