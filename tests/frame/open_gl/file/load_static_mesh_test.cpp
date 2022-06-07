#include "frame/open_gl/file/load_static_mesh_test.h"

#include "frame/level.h"
#include "frame/open_gl/file/load_static_mesh.h"

namespace test {

TEST_F(LoadStaticMeshTest, CreateLoadStaticMeshFromFileTest) {
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto maybe_node_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        level.get(), "Asset/Model/Monkey.obj", "Monkey");
    ASSERT_TRUE(maybe_node_vec);
    auto node_vec = maybe_node_vec.value();
    EXPECT_EQ(1, node_vec.size());
    auto node_id = node_vec.at(0);
    auto node    = level->GetSceneNodeFromId(node_id);
    ASSERT_TRUE(node);
    auto mesh_id = node->GetLocalMesh();
    EXPECT_NE(0, mesh_id);
    auto mesh = level->GetStaticMeshFromId(mesh_id);
    EXPECT_TRUE(mesh);
    EXPECT_LT(0, mesh->GetIndexSize());
}

TEST_F(LoadStaticMeshTest, CreateSceneFromFileTest) {
    auto level = std::make_unique<frame::Level>();
    ASSERT_TRUE(level);
    auto maybe_node_vec = frame::opengl::file::LoadStaticMeshesFromFile(
        level.get(), "Asset/Model/Scene.obj", "Scene");
    EXPECT_TRUE(maybe_node_vec);
    auto node_vec = maybe_node_vec.value();
    EXPECT_EQ(5, node_vec.size());
    for (auto node_id : node_vec) {
        auto node = level->GetSceneNodeFromId(node_id);
        EXPECT_TRUE(node);
        auto mesh_id     = node->GetLocalMesh();
        auto static_mesh = level->GetStaticMeshFromId(mesh_id);
        EXPECT_TRUE(static_mesh);
        EXPECT_LT(0, static_mesh->GetIndexSize());
        const auto point_id     = static_mesh->GetPointBufferId();
        const auto point_buffer = level->GetBufferFromId(point_id);
        EXPECT_LT(0, point_buffer->GetSize());
        const auto normal_id     = static_mesh->GetNormalBufferId();
        const auto normal_buffer = level->GetBufferFromId(normal_id);
        EXPECT_LT(0, normal_buffer->GetSize());
        const auto texture_id     = static_mesh->GetTextureBufferId();
        const auto texture_buffer = level->GetBufferFromId(texture_id);
        EXPECT_LT(0, texture_buffer->GetSize());
    }
}

}  // End namespace test.
