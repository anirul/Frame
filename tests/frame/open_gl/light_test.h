#pragma once

#include <gtest/gtest.h>

#include "frame/light_interface.h"
#include "frame/open_gl/light.h"

namespace test {

class LightTest : public testing::Test {
   public:
    LightTest() = default;

   protected:
    std::unique_ptr<frame::LightInterface> light_               = nullptr;
    std::unique_ptr<frame::opengl::LightManager> light_manager_ = nullptr;
};

}  // End namespace test.
