#pragma once

#include <gtest/gtest.h>
#include "Frame/Level.h"
#include "Frame/MaterialInterface.h"
#include "Frame/Window.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/Proto/ProtoLevelCreate.h"

namespace test {

	class ParseMaterialTest : public testing::Test
	{
	public:
		ParseMaterialTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			level_ = frame::proto::ParseLevelOpenGL(
				{ 320, 200 },
				frame::proto::GetLevel(),
				frame::proto::GetProgramFile(),
				frame::proto::GetSceneFile(),
				frame::proto::GetTextureFile(),
				{});
		}

	protected:
		std::shared_ptr<frame::LevelInterface> level_ = nullptr;
		std::shared_ptr<frame::MaterialInterface> material_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
