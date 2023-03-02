#pragma once

#include <gtest/gtest.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level.h"
#include "frame/opengl/renderer.h"
#include "frame/window_factory.h"

namespace test {

class RendererTest : public testing::Test {
   public:
    RendererTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

   public:
    bool LoadDefaultLevel() {
        auto level =
            frame::proto::ParseLevel(size_, frame::file::FindFile("asset/json/renderer_test.json"));
        if (level) {
            level_ = std::move(level);
            return true;
        }
        return false;
    }

   protected:
    const glm::uvec2 size_                             = { 320, 200 };
    std::unique_ptr<frame::WindowInterface> window_    = nullptr;
    std::unique_ptr<frame::LevelInterface> level_      = nullptr;
    std::unique_ptr<frame::opengl::Renderer> renderer_ = nullptr;
};

}  // End namespace test.
