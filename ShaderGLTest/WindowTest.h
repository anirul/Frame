#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"

namespace test {

	class WindowTest : public testing::Test
	{
	public:
		WindowTest() = default;

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
	};

} // End namespace test.