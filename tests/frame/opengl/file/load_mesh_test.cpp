#include "frame/opengl/file/load_mesh_test.h"

#include "frame/file/file_system.h"
#include "frame/level.h"
#include "frame/opengl/file/load_mesh.h"

namespace test
{

TEST_F(LoadMeshTest, CreateLoadMeshFromMonkeyGlbTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(),
        frame::file::FindFile("asset/model/monkey.glb"),
        "Monkey");
    ASSERT_TRUE(!node_vec.empty());
    EXPECT_EQ(1, node_vec.size());
    auto& [node_id, material_id] = node_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetMeshFromId(mesh_id);
    EXPECT_LT(0, mesh.GetIndexSize());
}

TEST_F(LoadMeshTest, CreateLoadMeshFromAppleGlbTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/apple.glb"), "Apple");
    ASSERT_TRUE(!node_vec.empty());
    EXPECT_EQ(1, node_vec.size());
    auto& [node_id, material_id] = node_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetMeshFromId(mesh_id);
    EXPECT_LT(0, mesh.GetIndexSize());
}

TEST_F(LoadMeshTest, CreateSceneFromFileTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/scene.glb"), "Scene");
    EXPECT_TRUE(!node_vec.empty());
    EXPECT_EQ(5, node_vec.size());
    for (const auto& [node_id, material_id] : node_vec)
    {
        auto& node = level->GetSceneNodeFromId(node_id);
        auto mesh_id = node.GetLocalMesh();
        auto& mesh = level->GetMeshFromId(mesh_id);
        EXPECT_LT(0, mesh.GetIndexSize());
        const auto point_id = mesh.GetPointBufferId();
        const auto& point_buffer = level->GetBufferFromId(point_id);
        EXPECT_LT(0, point_buffer.GetSize());
        const auto normal_id = mesh.GetNormalBufferId();
        const auto& normal_buffer = level->GetBufferFromId(normal_id);
        EXPECT_LT(0, normal_buffer.GetSize());
        const auto texture_id = mesh.GetTextureBufferId();
        const auto& texture_buffer = level->GetBufferFromId(texture_id);
        EXPECT_LT(0, texture_buffer.GetSize());
    }
}

} // End namespace test.



