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
			level_ = frame::proto::ParseLevelOpenGL(
				{ 320, 200 },
				frame::proto::GetLevel(),
				{},
				frame::proto::GetSceneFile(),
				frame::proto::GetTextureFile(),
				{});
		}

	protected:
		std::shared_ptr<frame::LevelInterface> level_ = nullptr;
		std::shared_ptr<frame::ProgramInterface> program_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
