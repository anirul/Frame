#pragma once

#include <gtest/gtest.h>

#include "frame/texture_interface.h"
#include "frame/window_factory.h"

namespace test {

class TextureCubeMapTest : public testing::Test {
   public:
    TextureCubeMapTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

   protected:
    const glm::uvec2 size_                            = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_   = nullptr;
    std::unique_ptr<frame::TextureInterface> texture_ = nullptr;
};

}  // End namespace test.
