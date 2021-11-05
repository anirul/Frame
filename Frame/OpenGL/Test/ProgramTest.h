#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/OpenGL/Program.h"

namespace test {

	class ProgramTest : public testing::Test
	{
	public:
		ProgramTest() {	window_ = frame::CreateSDLOpenGL({ 320, 200 });	}

	public:
		const std::string GetVertexSource() const;
		const std::string GetFragmentSource() const;

	protected:
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
	};

} // End namespace test.