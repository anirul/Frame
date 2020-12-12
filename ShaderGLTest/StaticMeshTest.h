#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/StaticMesh.h"

namespace test {

	class StaticMeshTest : public testing::Test
	{
	public:
		StaticMeshTest() { window_ = sgl::CreateSDLOpenGL({ 320, 200 }); }

	protected:
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		std::shared_ptr<sgl::StaticMesh> static_mesh_ = nullptr;
	};

} // End namespace test.
