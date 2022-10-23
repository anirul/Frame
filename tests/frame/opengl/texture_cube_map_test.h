#pragma once

#include <gtest/gtest.h>

#include "frame/texture_interface.h"
#include "frame/opengl/window.h"

namespace test {

class TextureCubeMapTest : public testing::Test {
   public:
    TextureCubeMapTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::TextureInterface> texture_   = nullptr;
};

}  // End namespace test.
