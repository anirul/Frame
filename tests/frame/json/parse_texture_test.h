#pragma once

#include <gtest/gtest.h>

#include "frame/texture_interface.h"
#include "frame/window.h"

namespace test {

class ParseTextureTest : public testing::Test {
   public:
    ParseTextureTest() { window_ = frame::CreateSDLOpenGL({ 320, 200 }); }

   protected:
    std::unique_ptr<frame::WindowInterface> window_   = nullptr;
    std::unique_ptr<frame::TextureInterface> texture_ = nullptr;
};

}  // End namespace test.
