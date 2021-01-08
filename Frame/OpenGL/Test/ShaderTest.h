#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/OpenGL/Shader.h"

namespace test {

	class ShaderTest : public testing::Test
	{
	public:
		ShaderTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::opengl::Shader> shader_ = nullptr;
	};

} // End namespace test.
