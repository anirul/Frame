#include "LightTest.h"

namespace test {

	TEST_F(LightTest, CreateLightTest)
	{
		EXPECT_FALSE(light_);
		light_ = std::make_shared<sgl::Light>(
			glm::vec3(1, 2, 3),
			glm::vec3(4, 5, 6));
		EXPECT_TRUE(light_);
	}

	TEST_F(LightTest, CheckValuesLightTest)
	{
		EXPECT_FALSE(light_);
		light_ = std::make_shared<sgl::Light>(
			glm::vec3(1, 2, 3),
			glm::vec3(4, 5, 6));
		EXPECT_TRUE(light_);
		EXPECT_EQ(glm::vec3(1, 2, 3), light_->GetPosition());
		EXPECT_EQ(glm::vec3(4, 5, 6), light_->GetColorIntensity());
	}

} // End namespace test.