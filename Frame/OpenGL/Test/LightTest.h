#pragma once

#include <gtest/gtest.h>
#include "Frame/OpenGL/Light.h"

namespace test {

	class LightTest : public testing::Test 
	{
	public:
		LightTest() = default;

	protected:
		std::shared_ptr<frame::opengl::LightInterface> light_ = nullptr;
		std::shared_ptr<frame::opengl::LightManager> light_manager_ = nullptr;
	};

} // End namespace test.
