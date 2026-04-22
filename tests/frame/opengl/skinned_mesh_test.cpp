#include "frame/opengl/skinned_mesh_test.h"

#include "frame/file/file_system.h"
#include "frame/level.h"
#include "frame/opengl/file/load_mesh.h"
#include "frame/opengl/skinned_mesh.h"

namespace test
{

namespace
{
constexpr const char* kSkinnedMeshAssetPath =
    "asset/model/cesium_man/CesiumMan.glb";
constexpr const char* kSkinnedMeshNodeName = "CesiumManMesh";
} // namespace

TEST_F(SkinnedMeshTest, LoadCesiumManGlbCreatesSkinnedMeshWithAnimationData)
{
    ASSERT_TRUE(window_);
    auto level = std::make_unique<frame::Level>();
    auto mesh_vec = frame::opengl::file::LoadMeshesFromFile(
        *level,
        frame::file::FindFile(kSkinnedMeshAssetPath),
        kSkinnedMeshNodeName);
    ASSERT_FALSE(mesh_vec.empty());

    frame::opengl::SkinnedMesh* skinned = nullptr;
    for (const auto& [node_id, material_id] : mesh_vec)
    {
        (void)material_id;
        auto& node = level->GetSceneNodeFromId(node_id);
        auto mesh_id = node.GetLocalMesh();
        ASSERT_NE(frame::NullId, mesh_id);
        auto& mesh = level->GetMeshFromId(mesh_id);
        skinned = dynamic_cast<frame::opengl::SkinnedMesh*>(&mesh);
        if (skinned)
        {
            break;
        }
    }

    ASSERT_NE(nullptr, skinned);
    EXPECT_TRUE(skinned->HasSkinning());
    EXPECT_NE(frame::NullId, skinned->GetTriangleBufferId());
    EXPECT_TRUE(skinned->HasRaytraceTriangleCallback());
    EXPECT_FALSE(skinned->HasRaytraceBvhCallback());

    skinned->SetSkinningAnimation(true, 1.5f);
    skinned->SetSkinningAnimationClip("", 0);
    EXPECT_TRUE(skinned->IsSkinningAnimationEnabled());
    EXPECT_FLOAT_EQ(1.5f, skinned->GetSkinningAnimationSpeed());
    EXPECT_EQ("", skinned->GetSkinningAnimationClipName());
    EXPECT_TRUE(skinned->GetSkinningAnimationClipIndex().has_value());
    EXPECT_DOUBLE_EQ(3.0, skinned->GetSkinningTime(2.0));

    auto matrices = skinned->EvaluateSkinning(skinned->GetSkinningTime(0.1));
    EXPECT_FALSE(matrices.empty());
    auto triangles =
        skinned->EvaluateRaytraceTriangles(skinned->GetSkinningTime(0.1));
    EXPECT_FALSE(triangles.empty());
}

TEST_F(SkinnedMeshTest, LoadCesiumManGlbWithBvhCreatesSkinnedBvhData)
{
    ASSERT_TRUE(window_);
    auto level = std::make_unique<frame::Level>();
    auto mesh_vec = frame::opengl::file::LoadMeshesFromFile(
        *level,
        frame::file::FindFile(kSkinnedMeshAssetPath),
        kSkinnedMeshNodeName,
        "",
        frame::proto::NodeMesh::BVH_ACCELERATION);
    ASSERT_FALSE(mesh_vec.empty());

    frame::opengl::SkinnedMesh* skinned = nullptr;
    for (const auto& [node_id, material_id] : mesh_vec)
    {
        (void)material_id;
        auto& node = level->GetSceneNodeFromId(node_id);
        auto mesh_id = node.GetLocalMesh();
        ASSERT_NE(frame::NullId, mesh_id);
        auto& mesh = level->GetMeshFromId(mesh_id);
        skinned = dynamic_cast<frame::opengl::SkinnedMesh*>(&mesh);
        if (skinned)
        {
            break;
        }
    }

    ASSERT_NE(nullptr, skinned);
    EXPECT_NE(frame::NullId, skinned->GetBvhBufferId());
    EXPECT_TRUE(skinned->HasRaytraceBvhCallback());

    auto bvh_nodes = skinned->EvaluateRaytraceBvh(0.0);
    EXPECT_FALSE(bvh_nodes.empty());
}

} // End namespace test.
