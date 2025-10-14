#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_json.h"
#include "frame/json/parse_level.h"
#include "frame/level_interface.h"
#include "frame/window_factory.h"

namespace test
{

class ParseProgramTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        proto_level_ = frame::json::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("asset/json/program_test.json"));

        try
        {
            window_ = frame::CreateNewWindow(frame::DrawingTargetEnum::NONE);
        }
        catch (const std::exception& ex)
        {
            GTEST_SKIP() << ex.what();
        }

        try
        {
            auto level = frame::json::ParseLevel(
                {320, 200},
                frame::file::FindFile("asset/json/program_test.json"));
            if (!level)
            {
                GTEST_SKIP() << "Couldn't parse level.";
            }
            level_ = std::move(level);
        }
        catch (const std::exception& ex)
        {
            GTEST_SKIP() << ex.what();
        }
    }

    frame::proto::Level proto_level_ = {};
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.
