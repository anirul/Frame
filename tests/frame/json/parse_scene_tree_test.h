#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_json.h"
#include "frame/level_interface.h"
#include "frame/node_interface.h"

namespace test
{

class ParseSceneTreeTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        proto_level_ = frame::json::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("asset/json/scene_tree_test.json"));
    }

    frame::proto::Level proto_level_ = {};
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
};

} // End namespace test.
