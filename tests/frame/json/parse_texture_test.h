#pragma once

#include <gtest/gtest.h>

#include "frame/opengl/window.h"
#include "frame/texture_interface.h"

namespace test {

class ParseTextureTest : public testing::Test {
   public:
    ParseTextureTest() { window_ = frame::opengl::CreateNoneOpenGL({ 320, 200 }); }

   protected:
    std::unique_ptr<frame::WindowInterface> window_   = nullptr;
    std::unique_ptr<frame::TextureInterface> texture_ = nullptr;
};

}  // End namespace test.
