#pragma once

#include <gtest/gtest.h>

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
            level_ = std::make_unique<frame::Level>();
        }
        catch (const std::exception& ex)
        {
            GTEST_SKIP() << ex.what();
        }
    }

    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
    std::unique_ptr<frame::MaterialInterface> material_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.
