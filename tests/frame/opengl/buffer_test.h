#pragma once

#include <gtest/gtest.h>

#include "frame/buffer_interface.h"
#include "frame/opengl/window.h"

namespace test {

class BufferTest : public testing::Test {
   public:
    BufferTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 330, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::BufferInterface> buffer_     = nullptr;
};

}  // End namespace test.
