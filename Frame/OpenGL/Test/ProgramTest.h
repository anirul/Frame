#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/OpenGL/Program.h"

namespace test {

	class ProgramTest : public testing::Test
	{
	public:
		ProgramTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::ProgramInterface> program_ = nullptr;
	};

} // End namespace test.