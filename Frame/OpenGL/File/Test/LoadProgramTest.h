#pragma once

#include <gtest/gtest.h>

#include "Frame/File/Obj.h"
#include "Frame/Window.h"

namespace test {

class LoadProgramTest : public testing::Test {
   public:
    LoadProgramTest() { window_ = frame::CreateSDLOpenGL({ 320, 200 }); }

   protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
