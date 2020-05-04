#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Render.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	class RenderTest : public testing::Test
	{
	public:
		RenderTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::Render> render_ = nullptr;
		std::shared_ptr<sgl::Window> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
	};

} // End namespace test.
