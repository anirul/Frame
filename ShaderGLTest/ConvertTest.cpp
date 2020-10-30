#include "ConvertTest.h"

namespace test {

	TEST_F(ConvertTest, ParseUniformTest)
	{
		frame::proto::Uniform uniform;
		uniform.set_name("test");
		uniform.set_uniform_float(1.0f);
		EXPECT_EQ(1.0f, sgl::ParseUniform(uniform.uniform_float()));
	}

	const sgl::Camera UnifromMock::GetCamera() const
	{
		return {};
	}

	const glm::mat4 UnifromMock::GetProjection() const
	{
		return {};
	}

	const glm::mat4 UnifromMock::GetView() const
	{
		return {};
	}

	const glm::mat4 UnifromMock::GetModel() const
	{
		return {};
	}

} // End namespace test.
