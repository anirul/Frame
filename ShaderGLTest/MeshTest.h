#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Mesh.h"

namespace test {

	class MeshTest : public testing::Test
	{
	public:
		MeshTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Mesh> mesh_ = nullptr;
	};

} // End namespace test.
