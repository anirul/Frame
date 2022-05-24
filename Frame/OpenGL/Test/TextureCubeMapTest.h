#pragma once

#include <gtest/gtest.h>

#include "Frame/TextureInterface.h"
#include "Frame/Window.h"

namespace test {

    class TextureCubeMapTest : public testing::Test
    {
    public:
        TextureCubeMapTest() : window_(frame::CreateSDLOpenGL(size_)) {}

    protected:
        const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
        std::unique_ptr<frame::WindowInterface> window_ = nullptr;
        std::unique_ptr<frame::TextureInterface> texture_ = nullptr;
    };

} // End namespace test.