#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Shader.h"

namespace test {

	class ShaderTest : public testing::Test
	{
	public:
		ShaderTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Shader> shader_ = nullptr;
	};

} // End namespace test.
