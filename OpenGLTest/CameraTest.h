#pragma once

#include <gtest/gtest.h>
#include "../Frame/CameraInterface.h"

namespace test {

	class CameraTest : public testing::Test
	{
	public:
		CameraTest() = default;

	protected:
		std::shared_ptr<frame::CameraInterface> camera_ = nullptr;
	};

} // End namespace test.
