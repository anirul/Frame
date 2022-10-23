#pragma once

#include <gtest/gtest.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/window.h"

namespace test {

class RendererTest : public testing::Test {
   public:
    RendererTest() : window_(frame::opengl::CreateNoneOpenGL(size_)) {}

   public:
    bool LoadDefaultLevel() {
        auto level =
            frame::proto::ParseLevel(size_, frame::file::FindFile("asset/json/renderer_test.json"),
                                     window_->GetUniqueDevice());
        if (level) {
            level_ = std::move(level);
            return true;
        }
        return false;
    }

   protected:
    std::pair<std::uint32_t, std::uint32_t> size_      = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_    = nullptr;
    std::unique_ptr<frame::LevelInterface> level_      = nullptr;
    std::unique_ptr<frame::opengl::Renderer> renderer_ = nullptr;
};

}  // End namespace test.
