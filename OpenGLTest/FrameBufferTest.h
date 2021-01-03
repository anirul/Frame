#pragma once

#include <gtest/gtest.h>
#include "../Frame/Error.h"
#include "../Frame/Window.h"
#include "../OpenGLLib/FrameBuffer.h"

namespace test {

	class FrameBufferTest : public testing::Test 
	{
	public:
		FrameBufferTest() 
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<frame::opengl::FrameBuffer> frame_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		frame::Error& error_ = frame::Error::GetInstance();
	};

} // End namespace test.
