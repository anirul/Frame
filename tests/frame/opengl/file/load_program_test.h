#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"
#include "frame/opengl/window.h"

namespace test {

class LoadProgramTest : public testing::Test {
   public:
    LoadProgramTest() : window_(frame::opengl::CreateNoneOpenGL({ 320, 200 })) {}

   protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
