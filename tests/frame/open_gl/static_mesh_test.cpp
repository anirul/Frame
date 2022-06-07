#include "frame/open_gl/static_mesh_test.h"

#include <GL/glew.h>

#include "frame/buffer_interface.h"
#include "frame/file/file_system.h"
#include "frame/level.h"
#include "frame/open_gl/file/load_static_mesh.h"

namespace test {

TEST_F(StaticMeshTest, CreateCubeMeshTest) {
    ASSERT_TRUE(window_);
    auto level          = std::make_unique<frame::Level>();
    auto maybe_mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        level.get(), frame::file::FindFile("Asset/Model/Cube.obj"), "cube");
    ASSERT_TRUE(maybe_mesh_vec);
    auto mesh_vec = maybe_mesh_vec.value();
    auto node_id  = mesh_vec.at(0);
    auto node     = level->GetSceneNodeFromId(node_id);
    ASSERT_TRUE(node);
    auto static_mesh_id = node->GetLocalMesh();
    EXPECT_NE(0, static_mesh_id);
    frame::StaticMeshInterface* static_mesh = level->GetStaticMeshFromId(static_mesh_id);
    EXPECT_EQ(1, mesh_vec.size());
    EXPECT_TRUE(static_mesh);
    EXPECT_EQ(0, static_mesh->GetMaterialId());
    EXPECT_NE(0, static_mesh->GetPointBufferId());
    EXPECT_NE(0, static_mesh->GetNormalBufferId());
    EXPECT_NE(0, static_mesh->GetTextureBufferId());
    EXPECT_NE(0, static_mesh->GetIndexBufferId());
    auto id           = static_mesh->GetIndexBufferId();
    auto index_buffer = level->GetBufferFromId(id);
    EXPECT_LE(18, index_buffer->GetSize());
    EXPECT_GE(144, index_buffer->GetSize());
    EXPECT_TRUE(static_mesh);
}

TEST_F(StaticMeshTest, CreateTorusMeshTest) {
    EXPECT_TRUE(window_);
    auto level          = std::make_unique<frame::Level>();
    auto maybe_mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        level.get(), frame::file::FindFile("Asset/Model/Torus.obj"), "torus");
    ASSERT_TRUE(maybe_mesh_vec);
    auto mesh_vec = maybe_mesh_vec.value();
    EXPECT_EQ(1, mesh_vec.size());
    auto node_id        = mesh_vec.at(0);
    auto node           = level->GetSceneNodeFromId(node_id);
    auto static_mesh_id = node->GetLocalMesh();
    EXPECT_NE(0, static_mesh_id);
    frame::StaticMeshInterface* static_mesh = level->GetStaticMeshFromId(static_mesh_id);
    ASSERT_TRUE(static_mesh);
    EXPECT_EQ(0, static_mesh->GetMaterialId());
    EXPECT_NE(0, static_mesh->GetPointBufferId());
    EXPECT_NE(0, static_mesh->GetNormalBufferId());
    EXPECT_NE(0, static_mesh->GetTextureBufferId());
    EXPECT_NE(0, static_mesh->GetIndexBufferId());
    auto id           = static_mesh->GetIndexBufferId();
    auto index_buffer = level->GetBufferFromId(id);
    EXPECT_LE(3456, index_buffer->GetSize());
    EXPECT_GE(13824, index_buffer->GetSize());
    EXPECT_TRUE(static_mesh);
}

}  // End namespace test.
