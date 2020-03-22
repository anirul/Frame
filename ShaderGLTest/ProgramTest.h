#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Program.h"

namespace test {

	class ProgramTest : public OpenGLTest
	{
	public:
		ProgramTest() : OpenGLTest()
		{
			GLContextAndGlewInit();
		}

	protected:
		std::shared_ptr<sgl::Program> program_;
	};

} // End namespace test.