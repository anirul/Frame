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
            glm::uvec2(320, 200), level_path_, asset_root_);
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

    for (const auto& program_info : level_data_.programs)
    {
        EXPECT_NE(built.level->GetIdFromName(program_info.name), frame::NullId);
    }

    const auto material_ids = built.level->GetMaterials();
    ASSERT_FALSE(material_ids.empty());
    for (const auto material_id : material_ids)
    {
        EXPECT_NE(material_id, frame::NullId);
    }
}

TEST_F(VulkanBuildLevelTest, BuildMeshVerticesFromStaticMeshInfo)
{
    ASSERT_FALSE(level_data_.meshes.empty());

    const auto& mesh_info = level_data_.meshes.front();
    const auto vertices = frame::vulkan::BuildMeshVertices(mesh_info);
    ASSERT_EQ(vertices.size(), mesh_info.positions.size() / 3);
    EXPECT_FLOAT_EQ(vertices.front().position.x, mesh_info.positions[0]);
    EXPECT_FLOAT_EQ(vertices.front().position.y, mesh_info.positions[1]);
    EXPECT_FLOAT_EQ(vertices.front().position.z, mesh_info.positions[2]);
    if (!mesh_info.normals.empty())
    {
        EXPECT_FLOAT_EQ(vertices.front().normal.x, mesh_info.normals[0]);
        EXPECT_FLOAT_EQ(vertices.front().normal.y, mesh_info.normals[1]);
        EXPECT_FLOAT_EQ(vertices.front().normal.z, mesh_info.normals[2]);
    }
    if (!mesh_info.uvs.empty())
    {
        EXPECT_FLOAT_EQ(vertices.front().uv.x, mesh_info.uvs[0]);
        EXPECT_FLOAT_EQ(vertices.front().uv.y, mesh_info.uvs[1]);
    }
}

} // namespace test
