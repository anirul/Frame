#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Mesh.h"

namespace test {

	class MeshTest : public OpenGLTest
	{
	public:
		MeshTest() : OpenGLTest()
		{
			GLContextAndGlewInit();
		}

	protected:
		std::shared_ptr<sgl::Mesh> mesh_ = nullptr;
	};

} // End namespace test.
