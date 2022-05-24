#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/OpenGL/FrameBuffer.h"

namespace test {

	class FrameBufferTest : public testing::Test 
	{
	public:
		FrameBufferTest() : window_(frame::CreateSDLOpenGL(size_)) {}

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::opengl::FrameBuffer> frame_ = nullptr;
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
