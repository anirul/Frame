#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/OpenGL/Program.h"

namespace test {

	class ProgramTest : public testing::Test
	{
	public:
		ProgramTest() : window_(frame::CreateSDLOpenGL(size_)) 
		{
			error_.SetWindowPtr(nullptr);
		}

	public:
		const std::string GetVertexSource() const;
		const std::string GetFragmentSource() const;

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		frame::Error& error_ = frame::Error::GetInstance();
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::ProgramInterface> program_ = nullptr;
	};

} // End namespace test.