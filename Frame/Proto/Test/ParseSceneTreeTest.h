#pragma once

#include <gtest/gtest.h>

#include "Frame/File/FileSystem.h"
#include "Frame/LevelInterface.h"
#include "Frame/NodeInterface.h"
#include "Frame/Window.h"

namespace test {

	class ParseSceneTreeTest: public testing::Test 
	{
	public:
		ParseSceneTreeTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			proto_level_ =
				frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
					frame::file::FindFile("Asset/Json/SceneTreeTest.json"));
		}

	protected:
		frame::proto::Level proto_level_ = {};
		std::unique_ptr<frame::LevelInterface> level_ = nullptr;
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
