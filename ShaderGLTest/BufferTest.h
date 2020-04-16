#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Buffer.h"

namespace test {

	class BufferTest : public testing::Test
	{
	public:
		BufferTest()
		{
			window_ = sgl::CreateSDLOpenGL({320, 200});
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Buffer> buffer_ = nullptr;
	};

} // End namespace test.
