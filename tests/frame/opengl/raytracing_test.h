#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_interface.h"
#include "frame/window_factory.h"

namespace test
{

class OpenGLRayTracingLevelTest : public ::testing::Test
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
    }

    std::unique_ptr<frame::LevelInterface> LoadLevel(
        const std::string& relative_path) const
    {
        return frame::json::ParseLevel(
            {1280u, 720u},
            frame::file::FindFile(relative_path));
    }

    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // namespace test
