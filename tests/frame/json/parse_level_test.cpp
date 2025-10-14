#include "frame/json/parse_level_test.h"

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/vulkan/json/parse_level.h"
#include "frame/vulkan/build_level.h"

namespace test
{

TEST_F(ParseLevelTest, BuildVulkanLevelFromJson)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_path =
        frame::file::FindFile("asset/json/level_test.json");
    const auto level_proto = frame::json::LoadLevelProto(level_path);
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200), level_proto, asset_root);

    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data);
    ASSERT_NE(built.level, nullptr);
    EXPECT_EQ(
        built.level->GetDefaultRootSceneNodeId(),
        built.level->GetIdFromName(level_proto.scene_tree().default_root_name()));
}

TEST_F(ParseLevelTest, CreateLevelDataFromPath)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/level_test.json"),
        asset_root);
    EXPECT_EQ(level_data.proto.name(), "LevelTest");
    ASSERT_EQ(level_data.textures.size(), 1u);
    EXPECT_EQ(level_data.textures.front().name, "DefaultTexture");
    EXPECT_EQ(level_data.asset_root, asset_root);
}

TEST_F(ParseLevelTest, CreateLevelDataFromPathVulkan)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_data = frame::vulkan::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/level_test.json"),
        asset_root);
    EXPECT_EQ(level_data.proto.name(), "LevelTest");
    ASSERT_EQ(level_data.textures.size(), 1u);
    EXPECT_EQ(level_data.textures.front().name, "DefaultTexture");
    EXPECT_EQ(level_data.asset_root, asset_root);
}

} // End namespace test.
