#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Convert.h"

namespace test {

	class UnifromMock : public sgl::UniformInterface
	{
	public:
		const sgl::Camera GetCamera() const override;
		const glm::mat4 GetProjection() const override;
		const glm::mat4 GetView() const override;
		const glm::mat4 GetModel() const override;
	};

	class ConvertTest : public ::testing::Test
	{
	public:
		ConvertTest() = default;

	protected:
		std::string test_uniform_ =
			R"json(
		{
          "name": "noize_scale",
          "value": {
            "vec2": {
              "x": "-4.0f",
              "y": "-4.0f"
            }
          }
        }
			)json";
	};

} // End namespace test.
