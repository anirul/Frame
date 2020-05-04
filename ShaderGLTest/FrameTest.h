#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Frame.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	class FrameTest : public testing::Test 
	{
	public:
		FrameTest() 
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::Frame> frame_ = nullptr;
		std::shared_ptr<sgl::Window> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
	};

} // End namespace test.
