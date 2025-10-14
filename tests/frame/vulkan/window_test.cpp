#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/vulkan/build_level.h"

namespace test
{

class VulkanBuildLevelTest : public ::testing::Test
{
  protected:
    VulkanBuildLevelTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile("asset/json/level_test.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(320, 200), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

TEST_F(VulkanBuildLevelTest, BuildLevelPopulatesDefaultScene)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data_);
    ASSERT_NE(built.level, nullptr);

    EXPECT_EQ(
        built.level->GetDefaultOutputTextureId(),
        built.level->GetIdFromName(level_proto_.default_texture_name()));

    const auto default_camera_name =
        level_proto_.scene_tree().default_camera_name();
    EXPECT_EQ(
        built.level->GetDefaultCameraId(),
        built.level->GetIdFromName(default_camera_name));

    const auto default_root_name =
        level_proto_.scene_tree().default_root_name();
    EXPECT_EQ(
        built.level->GetDefaultRootSceneNodeId(),
        built.level->GetIdFromName(default_root_name));
}

TEST_F(VulkanBuildLevelTest, BuildLevelRegistersProgramsMaterialsAndTextures)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 512), level_data_);
    ASSERT_NE(built.level, nullptr);

    for (const auto& proto_texture : level_proto_.textures())
    {
        EXPECT_NE(
            built.level->GetIdFromName(proto_texture.name()), frame::NullId);
    }

    for (const auto& proto_program : level_proto_.programs())
    {
        EXPECT_NE(
            built.level->GetIdFromName(proto_program.name()), frame::NullId);
    }

    for (const auto& proto_material : level_proto_.materials())
    {
        EXPECT_NE(
            built.level->GetIdFromName(proto_material.name()), frame::NullId);
    }
}

} // namespace test
