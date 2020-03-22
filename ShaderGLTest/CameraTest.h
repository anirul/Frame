#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Camera.h"

namespace test {

	class CameraTest : public testing::Test
	{
	public:
		CameraTest() = default;

	protected:
		std::shared_ptr<sgl::Camera> camera_ = nullptr;
	};

} // End namespace test.
