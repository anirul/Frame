#pragma once

#include <gtest/gtest.h>

#include "frame/level_interface.h"
#include "frame/window.h"

namespace test {

class ParseLevelTest : public testing::Test {
   public:
    ParseLevelTest() { window_ = frame::CreateSDLOpenGL({ 320, 200 }); }

   protected:
    std::shared_ptr<frame::WindowInterface> window_ = nullptr;
    std::shared_ptr<frame::LevelInterface> level_   = nullptr;
};

}  // End namespace test.
