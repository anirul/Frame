#pragma once

#include <gtest/gtest.h>
#include "Frame/TextureInterface.h"
#include "Frame/Window.h"

namespace test {

	class ParseTextureTest : public testing::Test 
	{
	public:
		ParseTextureTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::TextureInterface> texture_ = nullptr;
	};

} // End namespace test.
