#pragma once

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <imgui.h>
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

#include "frame/gui/window_glsl_file.h"
#include "frame/window_factory.h"

namespace test
{

class WindowGlslFileTest : public testing::Test
{
  public:
    WindowGlslFileTest()
        : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE))
    {
    }

  protected:
    void SetUp() override
    {
        ImGui::CreateContext();
        temp_file_ =
            std::filesystem::temp_directory_path() / "glsl_window_test.frag";
        std::ofstream out(temp_file_);
        out << "void main() {}";
        auto level = frame::json::ParseLevel(
            window_->GetDevice().GetSize(),
            frame::file::FindFile("asset/json/device_test.json"));
        window_->GetDevice().Startup(std::move(level));
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove(temp_file_, ec);
        ImGui::DestroyContext();
    }

    std::filesystem::path temp_file_;
    std::unique_ptr<frame::WindowInterface> window_;
};

} // namespace test
