#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	class TextureTest : public testing::Test
	{
	public:
		TextureTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
		std::shared_ptr<sgl::Texture> texture_ = nullptr;
		std::shared_ptr<sgl::TextureManager> texture_manager_ = nullptr;
	};

} // End namespace test.
