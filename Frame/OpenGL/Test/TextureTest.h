#pragma once

#include <gtest/gtest.h>
#include "Frame/Error.h"
#include "Frame/TextureInterface.h"
#include "Frame/Window.h"

namespace test {

	class TextureTest : public testing::Test
	{
	public:
		TextureTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		frame::Error& error_ = frame::Error::GetInstance();
		std::shared_ptr<frame::TextureInterface> texture_ = nullptr;
	};

} // End namespace test.
