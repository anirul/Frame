#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/FrameBuffer.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	class FrameBufferTest : public testing::Test 
	{
	public:
		FrameBufferTest() 
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::FrameBuffer> frame_ = nullptr;
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
	};

} // End namespace test.
