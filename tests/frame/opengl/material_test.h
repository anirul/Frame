#pragma once

#include <gtest/gtest.h>

#include "frame/material_interface.h"
#include "frame/opengl/material.h"
#include "frame/opengl/texture.h"
#include "frame/opengl/window.h"

namespace test {

class MaterialTest : public testing::Test {
   public:
    MaterialTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   protected:
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_     = nullptr;
    std::unique_ptr<frame::MaterialInterface> material_ = nullptr;
};

}  // End namespace test.
