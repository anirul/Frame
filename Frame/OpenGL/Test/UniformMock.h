#pragma once

#include <gmock/gmock.h>
#include "../Frame/UniformInterface.h"

namespace test {

	class UniformMock : public frame::UniformInterface
	{
	public:
		MOCK_METHOD(
			const frame::CameraInterface&, 
			GetCamera, 
			(), 
			(const, override));
		MOCK_METHOD(const glm::mat4, GetProjection, (), (const, override));
		MOCK_METHOD(const glm::mat4, GetView, (), (const, override));
		MOCK_METHOD(const glm::mat4, GetModel, (), (const, override));
		MOCK_METHOD(const double, GetDeltaTime, (), (const, override));
	};

} // End namespace test.
