#pragma once

#include <gtest/gtest.h>

#include "frame/json/parse_pixel.h"
#include "frame/level.h"
#include "frame/level_interface.h"
#include "frame/opengl/json/parse_texture.h"
#include "frame/window_factory.h"

namespace test
{

class ParseProgramTest : public testing::Test
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
            level_->SetDefaultTextureName("output");

            frame::proto::Texture input_texture;
            input_texture.set_name("Image");
            input_texture.mutable_size()->set_x(320);
            input_texture.mutable_size()->set_y(200);
            input_texture.mutable_pixel_element_size()->CopyFrom(
                frame::json::PixelElementSize_BYTE());
            input_texture.mutable_pixel_structure()->CopyFrom(
                frame::json::PixelStructure_RGB());
            auto parsed_input =
                frame::json::ParseTexture(input_texture, {320u, 200u});
            parsed_input->SetName(input_texture.name());
            ASSERT_NE(level_->AddTexture(std::move(parsed_input)), frame::NullId);

            frame::proto::Texture output_texture;
            output_texture.set_name("output");
            output_texture.mutable_size()->set_x(320);
            output_texture.mutable_size()->set_y(200);
            output_texture.mutable_pixel_element_size()->CopyFrom(
                frame::json::PixelElementSize_BYTE());
            output_texture.mutable_pixel_structure()->CopyFrom(
                frame::json::PixelStructure_RGB());
            auto parsed_output =
                frame::json::ParseTexture(output_texture, {320u, 200u});
            parsed_output->SetName(output_texture.name());
            ASSERT_NE(level_->AddTexture(std::move(parsed_output)), frame::NullId);
        }
        catch (const std::exception& ex)
        {
            GTEST_SKIP() << ex.what();
        }
    }

    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};

} // End namespace test.
