#pragma once

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_interface.h"
#include "frame/window.h"

namespace test {

class ParseProgramTest : public testing::Test {
   public:
    ParseProgramTest() {
        window_      = frame::CreateSDLOpenGL({ 320, 200 });
        proto_level_ = frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("asset/json/program_test.json"));
        auto maybe_level = frame::proto::ParseLevelOpenGL({ 320, 200 }, proto_level_);
        if (!maybe_level) throw std::runtime_error("Couldn't parse level.");
        level_ = std::move(maybe_level.value());
    }

   protected:
    frame::proto::Level proto_level_                  = {};
    std::unique_ptr<frame::LevelInterface> level_     = nullptr;
    std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
    std::unique_ptr<frame::WindowInterface> window_   = nullptr;
};

}  // End namespace test.
