#include "frame/opengl/file/load_static_mesh_test.h"

#include "frame/file/file_system.h"
#include "frame/level.h"
#include "frame/opengl/file/load_static_mesh.h"

namespace test
{

TEST_F(LoadStaticMeshTest, CreateLoadStaticMeshFromObjFileTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        *level.get(),
        frame::file::FindFile("asset/model/monkey.obj"),
        "Monkey");
    ASSERT_TRUE(!node_vec.empty());
    EXPECT_EQ(1, node_vec.size());
    auto node_id = node_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetStaticMeshFromId(mesh_id);
    EXPECT_LT(0, mesh.GetIndexSize());
}

TEST_F(LoadStaticMeshTest, CreateLoadStaticMeshFromPlyFileTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/apple.ply"), "Apple");
    ASSERT_TRUE(!node_vec.empty());
    EXPECT_EQ(1, node_vec.size());
    auto node_id = node_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetStaticMeshFromId(mesh_id);
    EXPECT_LT(0, mesh.GetIndexSize());
}

TEST_F(LoadStaticMeshTest, CreateSceneFromFileTest)
{
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto node_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/scene.obj"), "Scene");
    EXPECT_TRUE(!node_vec.empty());
    EXPECT_EQ(5, node_vec.size());
    for (auto node_id : node_vec)
    {
        auto& node = level->GetSceneNodeFromId(node_id);
        auto mesh_id = node.GetLocalMesh();
        auto& static_mesh = level->GetStaticMeshFromId(mesh_id);
        EXPECT_LT(0, static_mesh.GetIndexSize());
        const auto point_id = static_mesh.GetPointBufferId();
        const auto& point_buffer = level->GetBufferFromId(point_id);
        EXPECT_LT(0, point_buffer.GetSize());
        const auto normal_id = static_mesh.GetNormalBufferId();
        const auto& normal_buffer = level->GetBufferFromId(normal_id);
        EXPECT_LT(0, normal_buffer.GetSize());
        const auto texture_id = static_mesh.GetTextureBufferId();
        const auto& texture_buffer = level->GetBufferFromId(texture_id);
        EXPECT_LT(0, texture_buffer.GetSize());
    }
}

} // End namespace test.
