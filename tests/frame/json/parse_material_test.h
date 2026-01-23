#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level.h"
#include "frame/material_interface.h"
#include "frame/window_factory.h"

namespace test
{

class ParseMaterialTest : public testing::Test
{
  protected:
    void SetUp() override
    {
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
                frame::file::FindFile("asset/json/material_test.json"));
            if (!level)
            {
                GTEST_SKIP() << "Couldn't create level.";
            }
            level_ = std::move(level);
        }
        catch (const std::exception& ex)
        {
            GTEST_SKIP() << ex.what();
        }
    }

    std::shared_ptr<frame::LevelInterface> level_ = nullptr;
    std::shared_ptr<frame::MaterialInterface> material_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.
