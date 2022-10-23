#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/window.h"
#include "frame/static_mesh_interface.h"

namespace test {

class StaticMeshTest : public testing::Test {
   public:
    StaticMeshTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
};

}  // End namespace test.
