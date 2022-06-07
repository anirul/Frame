#pragma once

#include <gtest/gtest.h>

#include "Frame/File/Obj.h"
#include "Frame/Window.h"

namespace test {

class LoadTextureTest : public testing::Test {
   public:
    LoadTextureTest() { window_ = frame::CreateSDLOpenGL({ 320, 200 }); }

   protected:
    std::shared_ptr<frame::WindowInterface> window_;
};

}  // End namespace test.
