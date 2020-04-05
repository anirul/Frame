#include "CameraTest.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>

namespace test {

	TEST_F(CameraTest, CreationCameraTest)
	{
		EXPECT_FALSE(camera_);
		camera_ = std::make_shared<sgl::Camera>(glm::vec3(0.f, 0.f, 3.f));
		EXPECT_TRUE(camera_);
	}

	TEST_F(CameraTest, lookAtCameraTest)
	{
		EXPECT_FALSE(camera_);
		glm::vec3 position = { 0.f, 0.f, 3.f };
		glm::vec3 direction = { 0.f, 0.f, -1.f };
		glm::vec3 up = { 0.f, 1.f, 0.f };
		camera_ = std::make_shared<sgl::Camera>(position, direction, up);
		glm::mat4 look_at = glm::lookAt(position, direction - position, up);
		glm::mat4 camera_look_at = camera_->GetLookAt();
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				EXPECT_NEAR(look_at[i][j], camera_look_at[i][j], 1e-4);
			}
		}
		EXPECT_TRUE(camera_);
	}

} // End namespace test.
