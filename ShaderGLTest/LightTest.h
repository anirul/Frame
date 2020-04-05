#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Light.h"

namespace test {

	class LightTest : public testing::Test 
	{
	public:
		LightTest() = default;

	protected:
		std::shared_ptr<sgl::Light> light_ = nullptr;
	};

} // End namespace test.
