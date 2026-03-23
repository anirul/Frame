#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <glm/glm.hpp>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/scene_state.h"

namespace test
{

class VulkanSceneStateTest : public ::testing::Test
{
  protected:
    VulkanSceneStateTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile("asset/json/dragon.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(1280, 720), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

TEST_F(VulkanSceneStateTest, BuildsProjectionViewAndModelMatrices)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(640, 360), level_data_);
    ASSERT_NE(built.level, nullptr);

    const auto state = frame::vulkan::BuildSceneState(
        *built.level,
        frame::Logger::GetInstance(),
        {640u, 360u},
        /*elapsed_time_seconds=*/0.0f,
        built.level->GetIdFromName("RayTraceMaterial"),
        true);

    // Projection Y is flipped for Vulkan.
    EXPECT_LT(state.projection[1][1], 0.0f);
    EXPECT_NE(state.view, glm::mat4(1.0f));
    // Model may be identity if the mesh root is at the origin; ensure it's finite.
    EXPECT_TRUE(std::isfinite(state.model[0][0]));
}

TEST_F(VulkanSceneStateTest, CarriesLightInformation)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data_);
    ASSERT_NE(built.level, nullptr);

    const auto state = frame::vulkan::BuildSceneState(
        *built.level,
        frame::Logger::GetInstance(),
        {320u, 200u},
        0.0f,
        frame::NullId,
        true);

    // Direction is normalized in scene parsing.
    EXPECT_NEAR(state.light_dir.x, 0.7071f, 1e-3f);
    EXPECT_NEAR(state.light_dir.y, -0.7071f, 1e-3f);
    EXPECT_NEAR(state.light_dir.z, 0.0f, 1e-3f);
}

TEST_F(VulkanSceneStateTest, UsesExplicitPointLightSpecifier)
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

    auto directional_light = std::make_unique<frame::NodeLight>(
        func,
        frame::LightTypeEnum::DIRECTIONAL_LIGHT,
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        false);
    directional_light->SetName("sun");
    level.AddSceneNode(std::move(directional_light));

    auto camera = std::make_unique<frame::NodeCamera>(
        func,
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f));
    camera->SetName("camera");
    level.SetDefaultCameraName("camera");
    level.AddSceneNode(std::move(camera));

    auto point_light = std::make_unique<frame::NodeLight>(
        func,
        frame::LightTypeEnum::POINT_LIGHT,
        glm::vec3(2.0f, 3.0f, 4.0f),
        glm::vec3(1.0f, 0.8f, 0.6f),
        true);
    point_light->SetName("torch");
    level.AddSceneNode(std::move(point_light));

    const auto state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {320u, 200u},
        0.0f,
        frame::NullId,
        true);

    EXPECT_FLOAT_EQ(
        state.light_type,
        static_cast<float>(frame::LightTypeEnum::POINT_LIGHT));
    EXPECT_EQ(state.light_dir, glm::vec3(2.0f, 3.0f, 4.0f));
}

} // namespace test
