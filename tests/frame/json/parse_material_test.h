#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level.h"
#include "frame/material_interface.h"
#include "frame/window_factory.h"

namespace test {

class ParseMaterialTest : public testing::Test {
   public:
    ParseMaterialTest() : window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {
        auto level = frame::proto::ParseLevel(
            { 320, 200 }, frame::file::FindFile("asset/json/material_test.json"),
            window_->GetUniqueDevice());
        if (!level) throw std::runtime_error("Couldn't create level.");
        level_ = std::move(level);
    }

   protected:
    std::shared_ptr<frame::LevelInterface> level_       = nullptr;
    std::shared_ptr<frame::MaterialInterface> material_ = nullptr;
    std::shared_ptr<frame::WindowInterface> window_     = nullptr;
};

}  // End namespace test.
