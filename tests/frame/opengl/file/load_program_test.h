#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"
#include "frame/window_factory.h"

namespace test {

class LoadProgramTest : public testing::Test {
   public:
    LoadProgramTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

   protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
