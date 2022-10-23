#pragma once

#include <gmock/gmock.h>

#include "frame/uniform_interface.h"


namespace test {

class UniformMock : public frame::UniformInterface {
   public:
    MOCK_METHOD(const glm::vec3, GetCameraPosition, (), (const, override));
    MOCK_METHOD(const glm::vec3, GetCameraFront, (), (const, override));
    MOCK_METHOD(const glm::vec3, GetCameraRight, (), (const, override));
    MOCK_METHOD(const glm::vec3, GetCameraUp, (), (const, override));
    MOCK_METHOD(const glm::mat4, GetProjection, (), (const, override));
    MOCK_METHOD(const glm::mat4, GetView, (), (const, override));
    MOCK_METHOD(const glm::mat4, GetModel, (), (const, override));
    MOCK_METHOD(const double, GetDeltaTime, (), (const, override));
};

}  // End namespace test.
