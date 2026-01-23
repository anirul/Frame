#pragma once

#include <gtest/gtest.h>

#include "frame/texture_interface.h"
#include "frame/window_factory.h"

namespace test
{

class ParseTextureTest : public testing::Test
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

    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    std::unique_ptr<frame::TextureInterface> texture_ = nullptr;
};

} // End namespace test.
