#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <glm/glm.hpp>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/logger.h"
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
        level_path_ = frame::file::FindFile("asset/json/raytracing.json");
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
        built.level->GetIdFromName("RayTraceMaterial"));

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
        frame::NullId);

    // Direction is normalized in scene parsing.
    EXPECT_NEAR(state.light_dir.x, 0.7071f, 1e-3f);
    EXPECT_NEAR(state.light_dir.y, -0.7071f, 1e-3f);
    EXPECT_NEAR(state.light_dir.z, 0.0f, 1e-3f);
}

} // namespace test
