#pragma once

#include <gtest/gtest.h>

#include "Frame/Level.h"
#include "Frame/MaterialInterface.h"
#include "Frame/Window.h"
#include "Frame/Proto/ParseLevel.h"

namespace test {

	class ParseMaterialTest : public testing::Test
	{
	public:
		ParseMaterialTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			auto maybe_level = frame::proto::ParseLevelOpenGL(
				{ 320, 200 },
				frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
					"Asset/Json/MaterialTest.json"));
			if (!maybe_level) throw std::runtime_error("Couldn't create level.");
			level_ = std::move(maybe_level.value());
		}

	protected:
		std::shared_ptr<frame::LevelInterface> level_ = nullptr;
		std::shared_ptr<frame::MaterialInterface> material_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
