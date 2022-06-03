#pragma once

#include <gtest/gtest.h>
#include "Frame/Camera.h"

namespace test {

	class CameraTest : public testing::Test
	{
	public:
		CameraTest() = default;

	protected:
		std::shared_ptr<frame::Camera> camera_ = nullptr;
	};

} // End namespace test.
