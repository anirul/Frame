#pragma once

#include <gtest/gtest.h>
#include "Frame/Error.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/Window.h"

namespace test {

	class RenderBufferTest : public testing::Test
	{
	public:
		RenderBufferTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<frame::opengl::RenderBuffer> render_ = nullptr;
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		frame::Error& error_ = frame::Error::GetInstance();
	};

} // End namespace test.
