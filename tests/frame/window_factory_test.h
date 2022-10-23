#pragma once

#include <gtest/gtest.h>

#include "frame/window_factory.h"

namespace test {

class WindowFactoryTest : public testing::Test {
   public:
    WindowFactoryTest() = default;

   protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
