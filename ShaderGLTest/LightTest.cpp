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

	TEST_F(LightTest, CreateLightManagerTest)
	{
		EXPECT_FALSE(light_manager_);
		light_manager_ = std::make_shared<sgl::LightManager>();
		EXPECT_TRUE(light_manager_);
	}

	TEST_F(LightTest, AddLightToLightManagerLightTest)
	{
		EXPECT_FALSE(light_manager_);
		light_manager_ = std::make_shared<sgl::LightManager>();
		EXPECT_TRUE(light_manager_);
		EXPECT_EQ(0, light_manager_->GetLightCount());
		light_manager_->AddLight(
			sgl::Light(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)));
		EXPECT_EQ(1, light_manager_->GetLightCount());
		light_manager_->AddLight(
			sgl::Light(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)));
		EXPECT_EQ(2, light_manager_->GetLightCount());
		light_manager_->RemoveAllLights();
		EXPECT_EQ(0, light_manager_->GetLightCount());
	}

} // End namespace test.