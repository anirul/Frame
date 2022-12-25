#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/render_buffer.h"
#include "frame/window_factory.h"

namespace test {

class RenderBufferTest : public testing::Test {
   public:
    RenderBufferTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

   protected:
    const glm::uvec2 size_                               = { 320, 200 };
    std::unique_ptr<frame::opengl::RenderBuffer> render_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_      = nullptr;
};

}  // End namespace test.
