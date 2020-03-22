#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Buffer.h"

namespace test {

	class BufferTest : public OpenGLTest
	{
	public:
		BufferTest() : OpenGLTest()
		{
			GLContextAndGlewInit();
		}

	protected:
		std::shared_ptr<sgl::Buffer> buffer_ = nullptr;
	};

} // End namespace test.
