#include "frame/opengl/mesh_test.h"

#include <glad/glad.h>

#include "frame/buffer_interface.h"
#include "frame/file/file_system.h"
#include "frame/level.h"
#include "frame/opengl/file/load_mesh.h"

namespace test
{

TEST_F(MeshTest, CreateCubeMeshGlbTest)
{
    ASSERT_TRUE(window_);
    auto level = std::make_unique<frame::Level>();
    auto mesh_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/cube.glb"), "cube");
    ASSERT_TRUE(!mesh_vec.empty());
    auto& [node_id, material_id] = mesh_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetMeshFromId(mesh_id);
    EXPECT_EQ(1, mesh_vec.size());
    EXPECT_NE(0, mesh.GetPointBufferId());
    EXPECT_NE(0, mesh.GetNormalBufferId());
    EXPECT_NE(0, mesh.GetTextureBufferId());
    EXPECT_NE(0, mesh.GetIndexBufferId());
    auto id = mesh.GetIndexBufferId();
    auto& index_buffer = level->GetBufferFromId(id);
    EXPECT_LE(18, index_buffer.GetSize());
    EXPECT_GE(144, index_buffer.GetSize());
}

TEST_F(MeshTest, CreateTorusMeshGlbTest)
{
    EXPECT_TRUE(window_);
    auto level = std::make_unique<frame::Level>();
    auto mesh_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/torus.glb"), "torus");
    ASSERT_TRUE(!mesh_vec.empty());
    EXPECT_EQ(1, mesh_vec.size());
    auto& [node_id, material_id] = mesh_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetMeshFromId(mesh_id);
    EXPECT_NE(0, mesh.GetPointBufferId());
    EXPECT_NE(0, mesh.GetNormalBufferId());
    EXPECT_NE(0, mesh.GetTextureBufferId());
    EXPECT_NE(0, mesh.GetIndexBufferId());
    auto id = mesh.GetIndexBufferId();
    auto& index_buffer = level->GetBufferFromId(id);
    EXPECT_LE(3456, index_buffer.GetSize());
    EXPECT_GE(13824, index_buffer.GetSize());
}

TEST_F(MeshTest, CreateAppleMeshGlbTest)
{
    EXPECT_TRUE(window_);
    auto level = std::make_unique<frame::Level>();
    auto mesh_vec = frame::opengl::file::LoadMeshesFromFile(
        *level.get(), frame::file::FindFile("asset/model/apple.glb"), "apple");
    ASSERT_TRUE(!mesh_vec.empty());
    EXPECT_EQ(1, mesh_vec.size());
    auto& [node_id, material_id] = mesh_vec.at(0);
    auto& node = level->GetSceneNodeFromId(node_id);
    auto mesh_id = node.GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto& mesh = level->GetMeshFromId(mesh_id);
    EXPECT_NE(0, mesh.GetPointBufferId());
    EXPECT_NE(0, mesh.GetNormalBufferId());
    EXPECT_NE(0, mesh.GetTextureBufferId());
    EXPECT_NE(0, mesh.GetIndexBufferId());
    auto id = mesh.GetIndexBufferId();
    auto& index_buffer = level->GetBufferFromId(id);
    EXPECT_LE(5000, index_buffer.GetSize());
    EXPECT_GE(30000, index_buffer.GetSize());
}

} // End namespace test.



