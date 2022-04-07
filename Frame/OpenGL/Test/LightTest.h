#pragma once

#include <gtest/gtest.h>
#include "Frame/LightInterface.h"
#include "Frame/OpenGL/Light.h"

namespace test {

	class LightTest : public testing::Test 
	{
	public:
		LightTest() = default;

	protected:
		std::unique_ptr<frame::LightInterface> light_ = nullptr;
		std::unique_ptr<frame::opengl::LightManager> light_manager_ = nullptr;
	};

} // End namespace test.
