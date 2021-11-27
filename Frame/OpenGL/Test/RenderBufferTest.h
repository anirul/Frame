#pragma once

#include <gtest/gtest.h>
#include "Frame/Error.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/Window.h"

namespace test {

	class RenderBufferTest : public testing::Test
	{
	public:
		RenderBufferTest() : window_(frame::CreateSDLOpenGL(size_))
		{
			error_.SetWindowPtr(nullptr);
		}

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::opengl::RenderBuffer> render_ = nullptr;
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		frame::Error& error_ = frame::Error::GetInstance();
	};

} // End namespace test.
