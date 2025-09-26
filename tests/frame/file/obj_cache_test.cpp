#include <filesystem>

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/file/obj_cache.h"

namespace test
{

TEST(ObjCacheTest, RoundTrip)
{
    frame::file::ObjVertex vertex_a{};
    vertex_a.point = {1.0f, 2.0f, 3.0f};
    vertex_a.normal = {0.0f, 1.0f, 0.0f};
    vertex_a.tex_coord = {0.5f, 0.5f};

    frame::file::ObjVertex vertex_b{};
    vertex_b.point = {-1.0f, -2.0f, -3.0f};
    vertex_b.normal = {0.0f, -1.0f, 0.0f};
    vertex_b.tex_coord = {0.25f, 0.75f};

    std::vector<frame::file::ObjVertex> vertices = {vertex_a, vertex_b};
    std::vector<int> indices = {0, 1};
    frame::file::ObjMesh mesh(vertices, indices, 2, true);

    frame::file::ObjMaterial material;
    material.name = "material";
    material.ambient_vec4 = {0.1f, 0.2f, 0.3f, 0.4f};
    material.ambient_str = "ambient";
    material.diffuse_vec4 = {0.5f, 0.6f, 0.7f, 0.8f};
    material.diffuse_str = "diffuse";
    material.displacement_str = "disp";
    material.roughness_val = 0.9f;
    material.roughness_str = "rough";
    material.metallic_val = 0.25f;
    material.metallic_str = "metal";
    material.sheen_val = 0.75f;
    material.sheen_str = "sheen";
    material.emmissive_str = "emit";
    material.normal_str = "normal";

    frame::file::ObjCachePayload payload;
    payload.hasTextureCoordinates = true;
    payload.meshes.push_back(mesh);
    payload.materials.push_back(material);

    auto temp_dir =
        std::filesystem::temp_directory_path() / "frame_obj_cache_roundtrip";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    auto cache_path = temp_dir / "mesh.objpb";

    frame::file::ObjCacheMetadata metadata;
    metadata.cache_path = cache_path;
    metadata.cache_relative = frame::file::PurifyFilePath(cache_path);
    metadata.source_relative = "asset/model/dragon.obj";
    metadata.source_size = 100;
    metadata.source_mtime_ns = 42;

    frame::file::SaveObjCache(metadata, payload);
    auto loaded = frame::file::LoadObjCache(metadata);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->hasTextureCoordinates);
    ASSERT_EQ(loaded->meshes.size(), 1u);
    const auto& loaded_mesh = loaded->meshes[0];
    EXPECT_EQ(loaded_mesh.GetMaterialId(), mesh.GetMaterialId());
    EXPECT_EQ(
        loaded_mesh.HasTextureCoordinates(), mesh.HasTextureCoordinates());
    ASSERT_EQ(loaded_mesh.GetVertices().size(), vertices.size());
    EXPECT_NEAR(loaded_mesh.GetVertices()[0].point.x, vertex_a.point.x, 1e-5f);
    ASSERT_EQ(loaded_mesh.GetIndices().size(), indices.size());
    EXPECT_EQ(loaded_mesh.GetIndices()[1], indices[1]);

    ASSERT_EQ(loaded->materials.size(), 1u);
    const auto& loaded_material = loaded->materials[0];
    EXPECT_EQ(loaded_material.name, material.name);
    EXPECT_NEAR(loaded_material.ambient_vec4.w, material.ambient_vec4.w, 1e-5f);
    EXPECT_EQ(loaded_material.normal_str, material.normal_str);

    std::filesystem::remove_all(temp_dir);
}

TEST(ObjCacheTest, RejectsOutdatedMetadata)
{
    frame::file::ObjCachePayload payload;
    payload.hasTextureCoordinates = false;

    auto temp_dir =
        std::filesystem::temp_directory_path() / "frame_obj_cache_stale";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    auto cache_path = temp_dir / "mesh.objpb";

    frame::file::ObjCacheMetadata metadata;
    metadata.cache_path = cache_path;
    metadata.cache_relative = frame::file::PurifyFilePath(cache_path);
    metadata.source_relative = "asset/model/dragon.obj";
    metadata.source_size = 100;
    metadata.source_mtime_ns = 42;

    frame::file::SaveObjCache(metadata, payload);
    frame::file::ObjCacheMetadata stale = metadata;
    stale.source_size += 1;
    auto should_be_empty = frame::file::LoadObjCache(stale);
    EXPECT_FALSE(should_be_empty.has_value());

    std::filesystem::remove_all(temp_dir);
}

} // namespace test
