#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/mesh_utils.h"

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

TEST_F(VulkanBuildLevelTest, BuildMeshVerticesFromStaticMeshInfo)
{
    const auto mesh_level_path =
        frame::file::FindFile("asset/json/japanese_flag.json");
    const auto mesh_level_proto =
        frame::json::LoadLevelProto(mesh_level_path);
    const auto mesh_level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200), mesh_level_proto, asset_root_);
    ASSERT_FALSE(mesh_level_data.meshes.empty());

    const auto& mesh_info = mesh_level_data.meshes.front();
    const auto vertices = frame::vulkan::BuildMeshVertices(mesh_info);
    ASSERT_EQ(vertices.size(), mesh_info.positions.size() / 3);
    EXPECT_FLOAT_EQ(vertices.front().position.x, mesh_info.positions[0]);
    EXPECT_FLOAT_EQ(vertices.front().position.y, mesh_info.positions[1]);
    EXPECT_FLOAT_EQ(vertices.front().position.z, mesh_info.positions[2]);
    if (!mesh_info.uvs.empty())
    {
        EXPECT_FLOAT_EQ(vertices.front().uv.x, mesh_info.uvs[0]);
        EXPECT_FLOAT_EQ(vertices.front().uv.y, mesh_info.uvs[1]);
    }
}

} // namespace test
