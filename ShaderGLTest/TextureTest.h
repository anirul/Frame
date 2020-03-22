#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Texture.h"

namespace test {

	class TextureTest : public OpenGLTest
	{
	public:
		TextureTest() : OpenGLTest() 
		{
			GLContextAndGlewInit();
		}

	protected:
		std::shared_ptr<sgl::Texture> texture_ = nullptr;
		std::shared_ptr<sgl::TextureManager> texture_manager_ = nullptr;
	};

} // End namespace test.
