#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/level_interface.h"
#include "frame/node_interface.h"
#include "frame/window_factory.h"

namespace test {

class ParseSceneTreeTest : public testing::Test {
   public:
    ParseSceneTreeTest() {
        window_      = frame::CreateNewWindow(frame::DrawingTargetEnum::NONE);
        proto_level_ = frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("asset/json/scene_tree_test.json"));
    }

   protected:
    frame::proto::Level proto_level_                = {};
    std::unique_ptr<frame::LevelInterface> level_   = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

}  // End namespace test.
