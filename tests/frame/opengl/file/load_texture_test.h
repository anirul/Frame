#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"
#include "frame/window_factory.h"

namespace test {

	class LoadTextureTest : public testing::Test {
	public:
		LoadTextureTest()
			: window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

	protected:
		std::shared_ptr<frame::WindowInterface> window_;
	};

}  // End namespace test.
