#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Shader.h"

namespace test {

	class ShaderTest : public OpenGLTest
	{
	public:
		ShaderTest() : OpenGLTest()
		{
			GLContextAndGlewInit();
		}

	protected:
		std::shared_ptr<sgl::Shader> shader_ = nullptr;
	};

} // End namespace test.
