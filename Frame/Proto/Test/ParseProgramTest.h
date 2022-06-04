#pragma once

#include <gtest/gtest.h>

#include "Frame/File/FileSystem.h"
#include "Frame/LevelInterface.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/Window.h"

namespace test {

class ParseProgramTest : public testing::Test {
   public:
    ParseProgramTest() {
        window_      = frame::CreateSDLOpenGL({ 320, 200 });
        proto_level_ = frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
            frame::file::FindFile("Asset/Json/ProgramTest.json"));
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
