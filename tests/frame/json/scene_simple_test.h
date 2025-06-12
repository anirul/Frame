#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_json.h"
#include "frame/json/parse_level.h"
#include "frame/level_interface.h"
#include "frame/window_factory.h"

namespace test
{

class SceneSimpleTest : public testing::Test
{
  public:
    SceneSimpleTest()
    {
        window_ = frame::CreateNewWindow(frame::DrawingTargetEnum::NONE);
        proto_level_ = frame::json::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("asset/json/scene_simple.json"));
        auto level = frame::json::ParseLevel(
            {320, 200}, frame::file::FindFile("asset/json/scene_simple.json"));
        if (!level)
            throw std::runtime_error("Couldn't parse level.");
        level_ = std::move(level);
    }

  protected:
    frame::proto::Level proto_level_{};
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // namespace test
