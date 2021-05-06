#pragma once

#include <gtest/gtest.h>
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
		}

	protected:
		std::shared_ptr<frame::LevelInterface> level_ = nullptr;
		std::shared_ptr<frame::NodeInterface> node_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
