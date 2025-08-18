#include "frame/opengl/light_test.h"
#include "frame/level.h"
#include "frame/node_matrix.h"
#include "frame/node_light.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace test
{

TEST_F(LightTest, CreateLightTest)
{
    EXPECT_FALSE(light_);
    light_ = std::make_unique<frame::opengl::LightPoint>(
        glm::vec3(1, 2, 3), glm::vec3(4, 5, 6));
    EXPECT_TRUE(light_);
}

TEST_F(LightTest, CheckValuesLightTest)
{
    EXPECT_FALSE(light_);
    light_ = std::make_unique<frame::opengl::LightDirectional>(
        glm::vec3(1, 2, 3), glm::vec3(4, 5, 6));
    EXPECT_TRUE(light_);
    EXPECT_EQ(glm::vec3(1, 2, 3), light_->GetVector());
    EXPECT_EQ(glm::vec3(4, 5, 6), light_->GetColorIntensity());
}

TEST_F(LightTest, CreateLightManagerTest)
{
    EXPECT_FALSE(light_manager_);
    light_manager_ = std::make_unique<frame::opengl::LightManager>();
    EXPECT_TRUE(light_manager_);
}

TEST_F(LightTest, AddLightToLightManagerLightTest)
{
    EXPECT_FALSE(light_manager_);
    light_manager_ = std::make_unique<frame::opengl::LightManager>();
    EXPECT_TRUE(light_manager_);
    EXPECT_EQ(0, light_manager_->GetLightCount());
    light_manager_->AddLight(
        std::move(std::make_unique<frame::opengl::LightDirectional>(
            glm::vec3(0, 0, 0), glm::vec3(0, 0, 0))));
    EXPECT_EQ(1, light_manager_->GetLightCount());
    light_manager_->AddLight(
        std::move(std::make_unique<frame::opengl::LightPoint>(
            glm::vec3(0, 0, 0), glm::vec3(0, 0, 0))));
    EXPECT_EQ(2, light_manager_->GetLightCount());
    light_manager_->RemoveAllLights();
    EXPECT_EQ(0, light_manager_->GetLightCount());
}

TEST_F(LightTest, DirectionalLightUsesParentRotation)
{
    frame::Level level;
    auto func = [&level](const std::string& name) -> frame::NodeInterface* {
        auto id = level.GetIdFromName(name);
        if (id == frame::NullId)
        {
            return nullptr;
        }
        return &level.GetSceneNodeFromId(id);
    };

    glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    transform = glm::rotate(transform, glm::half_pi<float>(), glm::vec3(0, 1, 0));
    auto node_matrix = std::make_unique<frame::NodeMatrix>(func, transform, false);
    node_matrix->SetName("parent");

    auto node_light = std::make_unique<frame::NodeLight>(
        func,
        frame::LightTypeEnum::DIRECTIONAL_LIGHT,
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f));
    node_light->SetName("light");
    node_light->SetParentName("parent");

    level.AddSceneNode(std::move(node_matrix));
    level.AddSceneNode(std::move(node_light));

    auto lights = level.GetLights();
    ASSERT_EQ(1u, lights.size());
    auto& light_interface = level.GetLightFromId(lights[0]);
    auto* directional =
        dynamic_cast<frame::opengl::LightDirectional*>(&light_interface);
    ASSERT_NE(nullptr, directional);
    auto dir = directional->GetVector();
    EXPECT_NEAR(-1.0f, dir.x, 1e-5f);
    EXPECT_NEAR(0.0f, dir.y, 1e-5f);
    EXPECT_NEAR(0.0f, dir.z, 1e-5f);
    EXPECT_NEAR(1.0f, glm::length(dir), 1e-5f);
}

} // End namespace test.
