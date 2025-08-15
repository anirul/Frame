#include "frame/json/light_parent_test.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace test {

TEST_F(LightParentTest, DirectionalLightFollowParent)
{
    frame::Level level;
    auto functor = [&level](const std::string& name) -> frame::NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
            return nullptr;
        return &level.GetSceneNodeFromId(maybe_id);
    };

    auto root = std::make_unique<frame::NodeMatrix>(functor, glm::mat4(1.0f), false);
    root->GetData().set_name("root");
    level.AddSceneNode(std::move(root));

    auto rot = std::make_unique<frame::NodeMatrix>(functor, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), false);
    rot->GetData().set_name("rot");
    rot->SetParentName("root");
    level.AddSceneNode(std::move(rot));

    auto node_light = std::make_unique<frame::NodeLight>(
        functor,
        frame::LightTypeEnum::DIRECTIONAL_LIGHT,
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f));
    node_light->GetData().set_name("sun");
    node_light->SetParentName("rot");
    node_light->GetData().set_shadow_type(frame::proto::NodeLight::SOFT_SHADOW);
    level.AddSceneNode(std::move(node_light));

    auto light_ids = level.GetLights();
    ASSERT_EQ(1u, light_ids.size());
    auto& light = level.GetLightFromId(light_ids[0]);
    EXPECT_NEAR(0.0f, light.GetVector().x, 0.001f);
    EXPECT_NEAR(0.0f, light.GetVector().y, 0.001f);
    EXPECT_NEAR(-1.0f, light.GetVector().z, 0.001f);
}

} // namespace test
