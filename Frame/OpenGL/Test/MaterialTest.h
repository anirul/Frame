#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Material.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	class MaterialTest : public testing::Test
	{
	public:
		MaterialTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
		std::shared_ptr<sgl::Material> material_ = nullptr;
	};

} // End namespace test.