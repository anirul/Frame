#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"

namespace test {

	class TextureTest : public testing::Test
	{
	public:
		TextureTest()
		{
			window_ = sgl::MakeSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Texture> texture_ = nullptr;
		std::shared_ptr<sgl::TextureManager> texture_manager_ = nullptr;
	};

} // End namespace test.
