#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/LevelInterface.h"
#include "Frame/Proto/ProtoLevelCreate.h"
#include "Frame/Proto/ParseLevel.h"

namespace test {

	class ParseProgramTest : public testing::Test
	{
	public:
		ParseProgramTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			auto maybe_level = frame::proto::ParseLevelOpenGL(
				{ 320, 200 },
				frame::proto::GetLevel(),
				{},
				frame::proto::GetSceneFile(),
				frame::proto::GetTextureFile(),
				{});
			if (!maybe_level) throw std::runtime_error("Couldn't parse level.");
			level_ = std::move(maybe_level.value());
		}

	protected:
		std::shared_ptr<frame::LevelInterface> level_ = nullptr;
		std::shared_ptr<frame::ProgramInterface> program_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
