#pragma once

#include <gtest/gtest.h>

#include "Frame/LevelInterface.h"
#include "Frame/Window.h"

namespace test {

class ParseLevelTest : public testing::Test {
   public:
    ParseLevelTest() { window_ = frame::CreateSDLOpenGL({ 320, 200 }); }

   protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
    std::shared_ptr<frame::LevelInterface> level_   = nullptr;
};

}  // End namespace test.
