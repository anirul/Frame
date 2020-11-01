#pragma once

#include <gmock/gmock.h>
#include "../ShaderGLLib/Uniform.h"

namespace test {

	class UniformMock : public sgl::UniformInterface
	{
	public:
		MOCK_METHOD(const sgl::Camera, GetCamera, (), (const, override));
		MOCK_METHOD(const glm::mat4, GetProjection, (), (const, override));
		MOCK_METHOD(const glm::mat4, GetView, (), (const, override));
		MOCK_METHOD(const glm::mat4, GetModel, (), (const, override));
	};

} // End namespace test.
