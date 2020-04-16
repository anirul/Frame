#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Program.h"

namespace test {

	class ProgramTest : public testing::Test
	{
	public:
		ProgramTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Program> program_ = nullptr;
	};

} // End namespace test.