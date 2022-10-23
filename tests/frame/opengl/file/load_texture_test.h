#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"
#include "frame/opengl/window.h"

namespace test {

class LoadTextureTest : public testing::Test {
   public:
    LoadTextureTest() { window_ = frame::opengl::CreateNoneOpenGL({ 320, 200 }); }

   protected:
    std::shared_ptr<frame::WindowInterface> window_;
};

}  // End namespace test.
