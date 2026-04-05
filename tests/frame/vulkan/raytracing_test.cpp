#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <gtest/gtest.h>
#include <vector>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/camera.h"
#include "frame/logger.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/skinned_mesh.h"
#include <glm/glm.hpp>

namespace test
{

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Triangle
{
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

struct BvhNode
{
    glm::vec4 min;
    glm::vec4 max;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

constexpr std::size_t kFloatsPerVertex = 12;
constexpr std::size_t kFloatsPerTriangle = kFloatsPerVertex * 3;

bool EndsWith(const std::string& value, const std::string& suffix)
{
    if (suffix.size() > value.size())
    {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const frame::json::ProgramInfo* FindProgramInfoByName(
    const frame::json::LevelData& level_data,
    const std::string& name)
{
    for (const auto& program_info : level_data.programs)
    {
        if (program_info.name == name)
        {
            return &program_info;
        }
    }
    return nullptr;
}

frame::EntityId FindBufferByInnerName(
    const frame::LevelInterface& level,
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto& name : material.GetBufferNames())
    {
        if (material.GetInnerBufferName(name) != expected_inner_name)
        {
            continue;
        }
        return level.GetIdFromName(name);
    }
    return frame::NullId;
}

frame::EntityId FindMaterialForNode(
    const frame::LevelInterface& level,
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum,
    const std::string& node_name)
{
    for (const auto& [node_id, material_id] : level.GetMeshMaterialIds(render_time_enum))
    {
        if (level.GetNameFromId(node_id) == node_name)
        {
            return material_id;
        }
    }
    return frame::NullId;
}

frame::EntityId FindTextureByInnerName(
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto texture_id : material.GetTextureIds())
    {
        if (material.GetInnerName(texture_id) == expected_inner_name)
        {
            return texture_id;
        }
    }
    return frame::NullId;
}

float ReadTextureFirstChannel(
    const frame::LevelInterface& level, frame::EntityId texture_id)
{
    if (!texture_id)
    {
        return 0.0f;
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT: {
        const auto data = texture.GetTextureFloat();
        return data.empty() ? 0.0f : data.front();
    }
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF: {
        const auto data = texture.GetTextureWord();
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 65535.0f;
    }
    case frame::proto::PixelElementSize::BYTE:
    default: {
        const auto data = texture.GetTextureByte();
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 255.0f;
    }
    }
}

std::array<float, 3> ReadTextureRgb(
    const frame::LevelInterface& level, frame::EntityId texture_id)
{
    if (!texture_id)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT: {
        const auto data = texture.GetTextureFloat();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {data[0], data[1], data[2]};
    }
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF: {
        const auto data = texture.GetTextureWord();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {
            static_cast<float>(data[0]) / 65535.0f,
            static_cast<float>(data[1]) / 65535.0f,
            static_cast<float>(data[2]) / 65535.0f};
    }
    case frame::proto::PixelElementSize::BYTE:
    default: {
        const auto data = texture.GetTextureByte();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {
            static_cast<float>(data[0]) / 255.0f,
            static_cast<float>(data[1]) / 255.0f,
            static_cast<float>(data[2]) / 255.0f};
    }
    }
}

Triangle ReadTriangle(const float* data, std::size_t tri_index)
{
    auto read_vertex = [&](std::size_t base) {
        Vertex vertex{};
        vertex.position = glm::vec3(
            data[base + 0],
            data[base + 1],
            data[base + 2]);
        vertex.normal = glm::vec3(
            data[base + 4],
            data[base + 5],
            data[base + 6]);
        vertex.uv = glm::vec2(
            data[base + 8],
            data[base + 9]);
        return vertex;
    };
    const std::size_t base = tri_index * kFloatsPerTriangle;
    Triangle tri{};
    tri.v0 = read_vertex(base);
    tri.v1 = read_vertex(base + kFloatsPerVertex);
    tri.v2 = read_vertex(base + kFloatsPerVertex * 2);
    return tri;
}

void ExpectTriangleBufferValid(const frame::vulkan::Buffer& buffer)
{
    const auto& tri_bytes = buffer.GetRawData();
    ASSERT_FALSE(tri_bytes.empty());
    ASSERT_EQ(tri_bytes.size() % sizeof(float), 0u);
    ASSERT_EQ(tri_bytes.size() % (sizeof(float) * kFloatsPerTriangle), 0u);

    const std::size_t tri_count =
        tri_bytes.size() / (sizeof(float) * kFloatsPerTriangle);
    ASSERT_GT(tri_count, 0u);

    const float* tri_floats =
        reinterpret_cast<const float*>(tri_bytes.data());
    Triangle first_tri = ReadTriangle(tri_floats, 0);
    EXPECT_TRUE(std::isfinite(first_tri.v0.position.x));
    EXPECT_TRUE(std::isfinite(first_tri.v0.position.y));
    EXPECT_TRUE(std::isfinite(first_tri.v0.position.z));
    EXPECT_TRUE(std::isfinite(first_tri.v0.normal.x));
    EXPECT_TRUE(std::isfinite(first_tri.v0.normal.y));
    EXPECT_TRUE(std::isfinite(first_tri.v0.normal.z));

    glm::vec3 min_pos(std::numeric_limits<float>::max());
    glm::vec3 max_pos(std::numeric_limits<float>::lowest());
    for (std::size_t i = 0; i < tri_count; ++i)
    {
        Triangle tri = ReadTriangle(tri_floats, i);
        min_pos = glm::min(min_pos, tri.v0.position);
        min_pos = glm::min(min_pos, tri.v1.position);
        min_pos = glm::min(min_pos, tri.v2.position);
        max_pos = glm::max(max_pos, tri.v0.position);
        max_pos = glm::max(max_pos, tri.v1.position);
        max_pos = glm::max(max_pos, tri.v2.position);
    }
    glm::vec3 extent = max_pos - min_pos;
    EXPECT_TRUE(std::isfinite(extent.x));
    EXPECT_TRUE(std::isfinite(extent.y));
    EXPECT_TRUE(std::isfinite(extent.z));
    EXPECT_TRUE(extent.x > 0.0f || extent.y > 0.0f || extent.z > 0.0f);
}

class VulkanRayTracingParseTest : public ::testing::Test
{
  protected:
    VulkanRayTracingParseTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile("asset/json/dragon.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(512, 288), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

class VulkanSkinnedRayTracingParseTest : public ::testing::Test
{
  protected:
    VulkanSkinnedRayTracingParseTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile("asset/json/skinned_mesh.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(512, 288), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

class VulkanRayTracingDualParseTest : public ::testing::Test
{
  protected:
    VulkanRayTracingDualParseTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile("asset/json/raytracing.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(512, 288), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

class VulkanRayTracingSceneSourceParseTest : public ::testing::Test
{
  protected:
    VulkanRayTracingSceneSourceParseTest()
    {
        asset_root_ = frame::file::FindDirectory("asset");
        level_path_ = frame::file::FindFile(
            "asset/json/raytracing_scene_source.json");
        level_proto_ = frame::json::LoadLevelProto(level_path_);
        level_data_ = frame::json::ParseLevelData(
            glm::uvec2(512, 288), level_proto_, asset_root_);
    }

    std::filesystem::path asset_root_;
    std::filesystem::path level_path_;
    frame::proto::Level level_proto_;
    frame::json::LevelData level_data_;
};

TEST_F(VulkanRayTracingDualParseTest, BuildsTriangleBuffersFromScene)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const std::vector<std::string> expected_buffers = {
        "TriangleBufferTransmissive",
        "BvhBufferTransmissive",
        "TriangleBufferOpaque",
        "BvhBufferOpaque"};
    for (const auto& inner_name : expected_buffers)
    {
        auto id = FindBufferByInnerName(level, material, inner_name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << inner_name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << inner_name;
        EXPECT_GT(buffer->GetSize(), 0u) << "Empty buffer " << inner_name;
    }
}

TEST_F(
    VulkanRayTracingSceneSourceParseTest,
    SceneRenderTimeSourceMeshGetsPreprocessMaterialAndAutoResolve)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto scene_cube_material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "SceneCube");
    ASSERT_NE(scene_cube_material_id, frame::NullId);
    const auto& scene_cube_material = level.GetMaterialFromId(
        scene_cube_material_id);
    EXPECT_NE(level.GetNameFromId(scene_cube_material_id), "RayTraceMaterial");
    EXPECT_NE(
        scene_cube_material.GetPreprocessProgramId(&level),
        frame::NullId);

    const auto resolve_material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "RayTracingRendering");
    ASSERT_NE(resolve_material_id, frame::NullId);
    EXPECT_EQ(resolve_material_id, level.GetIdFromName("RayTraceMaterial"));
    const auto& resolve_material = level.GetMaterialFromId(resolve_material_id);
    EXPECT_EQ(resolve_material.GetPreprocessProgramId(&level), frame::NullId);
}

TEST_F(
    VulkanRayTracingSceneSourceParseTest,
    SceneRenderTimeSourceMeshBuildsAggregateBuffersOnResolveMaterial)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto resolve_material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(resolve_material_id, frame::NullId);
    const auto& resolve_material = level.GetMaterialFromId(resolve_material_id);
    const auto triangle_id = FindBufferByInnerName(
        level, resolve_material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);
    EXPECT_GT(triangle_buffer->GetSize(), 0u);
}

TEST_F(VulkanRayTracingDualParseTest, UsesHardwareRaytracingStageFiles)
{
    const auto* program_info = FindProgramInfoByName(
        level_data_, "RayTraceProgram");
    ASSERT_NE(program_info, nullptr);
    EXPECT_EQ(program_info->vulkan.raygen_shader, "raytrace.rgen");
    EXPECT_EQ(program_info->vulkan.miss_shader, "raytrace.rmiss");
    EXPECT_EQ(program_info->vulkan.closesthit_shader, "raytrace.rchit");
}

TEST_F(VulkanRayTracingDualParseTest, SceneMaterialCarriesAllSharedBindings)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level.GetMaterialFromId(material_id);
    std::vector<std::string> actual_texture_bindings = {};
    for (const auto texture_id : material.GetTextureIds())
    {
        actual_texture_bindings.push_back(material.GetInnerName(texture_id));
    }
    SCOPED_TRACE(::testing::Message()
                 << "Actual scene texture bindings: "
                 << ::testing::PrintToString(actual_texture_bindings));

    const std::vector<std::string> expected_textures = {
        "opaque_albedo_texture",
        "opaque_normal_texture",
        "opaque_roughness_texture",
        "opaque_metallic_texture",
        "opaque_ao_texture",
        "opaque_specular_factor_texture",
        "opaque_specular_color_texture",
        "transmissive_albedo_texture",
        "transmissive_normal_texture",
        "transmissive_roughness_texture",
        "transmissive_metallic_texture",
        "transmissive_ao_texture",
        "transmissive_transmission_texture",
        "transmissive_ior_texture",
        "transmissive_thickness_texture",
        "transmissive_attenuation_color_texture",
        "transmissive_attenuation_distance_texture",
        "skybox",
        "skybox_env"};
    for (const auto& inner_name : expected_textures)
    {
        EXPECT_NE(FindTextureByInnerName(material, inner_name), frame::NullId)
            << "Missing texture binding " << inner_name;
    }

    const std::vector<std::string> expected_buffers = {
        "TriangleBufferTransmissive",
        "BvhBufferTransmissive",
        "TriangleBufferOpaque",
        "BvhBufferOpaque"};
    for (const auto& inner_name : expected_buffers)
    {
        EXPECT_NE(FindBufferByInnerName(level, material, inner_name), frame::NullId)
            << "Missing buffer binding " << inner_name;
    }
}

TEST_F(VulkanRayTracingDualParseTest, StaticRaytraceSceneStateUsesMeshHolderTransform)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);

    const auto scene_state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {512u, 288u},
        1.0f,
        material_id,
        false,
        "mesh_holder");

    EXPECT_GT(std::abs(scene_state.model[0][2]), 1.0e-4f);
    EXPECT_GT(std::abs(scene_state.env_map_model[0][2]), 1.0e-4f);
}

TEST_F(VulkanRayTracingDualParseTest, HardwarePreferredBuildSkipsCpuBvhData)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_,
        {.prefer_hardware_raytracing = true});
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const auto transmissive_triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferTransmissive");
    const auto transmissive_bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferTransmissive");
    const auto opaque_triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    const auto opaque_bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(transmissive_triangle_id, frame::NullId);
    ASSERT_NE(transmissive_bvh_id, frame::NullId);
    ASSERT_NE(opaque_triangle_id, frame::NullId);
    ASSERT_NE(opaque_bvh_id, frame::NullId);

    auto* transmissive_triangle = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(transmissive_triangle_id));
    auto* transmissive_bvh = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(transmissive_bvh_id));
    auto* opaque_triangle = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(opaque_triangle_id));
    auto* opaque_bvh = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(opaque_bvh_id));
    ASSERT_NE(transmissive_triangle, nullptr);
    ASSERT_NE(transmissive_bvh, nullptr);
    ASSERT_NE(opaque_triangle, nullptr);
    ASSERT_NE(opaque_bvh, nullptr);

    EXPECT_GT(transmissive_triangle->GetSize(), 0u);
    EXPECT_GT(opaque_triangle->GetSize(), 0u);
    EXPECT_EQ(transmissive_bvh->GetSize(), 0u);
    EXPECT_EQ(opaque_bvh->GetSize(), 0u);
}

TEST_F(VulkanRayTracingDualParseTest, TriangleDataLooksValid)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    for (const auto& inner_name :
         {"TriangleBufferTransmissive", "TriangleBufferOpaque"})
    {
        auto id = FindBufferByInnerName(level, material, inner_name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << inner_name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << inner_name;
        ExpectTriangleBufferValid(*buffer);
    }
}

TEST_F(VulkanRayTracingDualParseTest, ImportsGltfGlassMaterialFromIco)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "Ico");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level.GetMaterialFromId(material_id);

    const auto transmission_texture_id = FindTextureByInnerName(
        material, "transmission_texture");
    const auto ior_texture_id = FindTextureByInnerName(material, "ior_texture");
    const auto thickness_texture_id = FindTextureByInnerName(
        material, "thickness_texture");
    const auto attenuation_color_texture_id = FindTextureByInnerName(
        material, "attenuation_color_texture");
    const auto attenuation_distance_texture_id = FindTextureByInnerName(
        material, "attenuation_distance_texture");
    const auto albedo_texture_id = FindTextureByInnerName(
        material, "albedo_texture");
    const auto normal_texture_id = FindTextureByInnerName(
        material, "normal_texture");
    const auto roughness_texture_id = FindTextureByInnerName(
        material, "roughness_texture");
    const auto metallic_texture_id = FindTextureByInnerName(
        material, "metallic_texture");
    const auto ao_texture_id = FindTextureByInnerName(
        material, "ao_texture");

    ASSERT_NE(transmission_texture_id, frame::NullId);
    ASSERT_NE(ior_texture_id, frame::NullId);
    ASSERT_NE(thickness_texture_id, frame::NullId);
    ASSERT_NE(attenuation_color_texture_id, frame::NullId);
    ASSERT_NE(attenuation_distance_texture_id, frame::NullId);
    ASSERT_NE(albedo_texture_id, frame::NullId);
    ASSERT_NE(normal_texture_id, frame::NullId);
    ASSERT_NE(roughness_texture_id, frame::NullId);
    ASSERT_NE(metallic_texture_id, frame::NullId);
    ASSERT_NE(ao_texture_id, frame::NullId);
    EXPECT_NE(albedo_texture_id, level.GetIdFromName("albedo_texture"));
    EXPECT_NE(normal_texture_id, level.GetIdFromName("normal_texture"));
    EXPECT_NE(roughness_texture_id, level.GetIdFromName("roughness_texture"));
    EXPECT_NE(metallic_texture_id, level.GetIdFromName("metallic_texture"));
    EXPECT_NE(ao_texture_id, level.GetIdFromName("ao_texture"));

    EXPECT_NEAR(
        ReadTextureFirstChannel(level, transmission_texture_id),
        1.0f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(level, ior_texture_id),
        1.45f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(level, thickness_texture_id),
        0.35f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(level, attenuation_distance_texture_id),
        1.5f,
        0.02f);

    const auto attenuation_color = ReadTextureRgb(
        level, attenuation_color_texture_id);
    EXPECT_NEAR(attenuation_color[0], 0.82f, 0.03f);
    EXPECT_NEAR(attenuation_color[1], 0.97f, 0.03f);
    EXPECT_NEAR(attenuation_color[2], 0.90f, 0.03f);
}

TEST_F(VulkanRayTracingDualParseTest, ImportsGltfOpaqueSpecularFromPlate)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto plate_material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "Plate");
    ASSERT_NE(plate_material_id, frame::NullId);
    const auto scene_material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(scene_material_id, frame::NullId);

    const auto& plate_material = level.GetMaterialFromId(plate_material_id);
    const auto& scene_material = level.GetMaterialFromId(scene_material_id);
    const auto plate_specular_factor_id = FindTextureByInnerName(
        plate_material, "specular_factor_texture");
    const auto plate_specular_color_id = FindTextureByInnerName(
        plate_material, "specular_color_texture");
    const auto plate_albedo_id = FindTextureByInnerName(
        plate_material, "albedo_texture");
    const auto plate_normal_id = FindTextureByInnerName(
        plate_material, "normal_texture");
    const auto plate_roughness_id = FindTextureByInnerName(
        plate_material, "roughness_texture");
    const auto plate_metallic_id = FindTextureByInnerName(
        plate_material, "metallic_texture");
    const auto plate_ao_id = FindTextureByInnerName(
        plate_material, "ao_texture");
    const auto scene_specular_factor_id = FindTextureByInnerName(
        scene_material, "opaque_specular_factor_texture");
    const auto scene_specular_color_id = FindTextureByInnerName(
        scene_material, "opaque_specular_color_texture");

    ASSERT_NE(plate_specular_factor_id, frame::NullId);
    ASSERT_NE(plate_specular_color_id, frame::NullId);
    ASSERT_NE(plate_albedo_id, frame::NullId);
    ASSERT_NE(plate_normal_id, frame::NullId);
    ASSERT_NE(plate_roughness_id, frame::NullId);
    ASSERT_NE(plate_metallic_id, frame::NullId);
    ASSERT_NE(plate_ao_id, frame::NullId);
    ASSERT_NE(scene_specular_factor_id, frame::NullId);
    ASSERT_NE(scene_specular_color_id, frame::NullId);
    EXPECT_EQ(scene_specular_factor_id, plate_specular_factor_id);
    EXPECT_EQ(scene_specular_color_id, plate_specular_color_id);
    EXPECT_EQ(plate_albedo_id, level.GetIdFromName("albedo_texture"));
    EXPECT_EQ(plate_normal_id, level.GetIdFromName("normal_texture"));
    EXPECT_EQ(plate_roughness_id, level.GetIdFromName("roughness_texture"));
    EXPECT_EQ(plate_metallic_id, level.GetIdFromName("metallic_texture"));
    EXPECT_EQ(plate_ao_id, level.GetIdFromName("ao_texture"));
    EXPECT_NEAR(
        ReadTextureFirstChannel(level, plate_specular_factor_id),
        1.0f,
        0.02f);

    const auto specular_color = ReadTextureRgb(level, plate_specular_color_id);
    EXPECT_NEAR(specular_color[0], 0.0f, 0.02f);
    EXPECT_NEAR(specular_color[1], 0.0f, 0.02f);
    EXPECT_NEAR(specular_color[2], 0.0f, 0.02f);
}

TEST_F(
    VulkanSkinnedRayTracingParseTest,
    HardwarePreferredBuildSkipsCpuBvhForAnimatedScene)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_,
        {.prefer_hardware_raytracing = true});
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    const auto bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);

    auto* triangle_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    auto* bvh_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(bvh_id));
    ASSERT_NE(triangle_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);
    EXPECT_GT(triangle_buffer->GetSize(), 0u);
    EXPECT_EQ(bvh_buffer->GetSize(), 0u);

    const auto fox_node_id = level.GetIdFromName("FoxMesh");
    ASSERT_NE(fox_node_id, frame::NullId);
    auto& fox_node = level.GetSceneNodeFromId(fox_node_id);
    const auto fox_mesh_id = fox_node.GetLocalMesh();
    ASSERT_NE(fox_mesh_id, frame::NullId);
    auto* skinned_mesh = dynamic_cast<frame::vulkan::SkinnedMesh*>(
        &level.GetMeshFromId(fox_mesh_id));
    ASSERT_NE(skinned_mesh, nullptr);
    EXPECT_TRUE(skinned_mesh->HasRaytraceTriangleCallback());
    EXPECT_FALSE(skinned_mesh->HasRaytraceBvhCallback());
    EXPECT_EQ(skinned_mesh->GetBvhBufferId(), frame::NullId);
}

TEST_F(VulkanSkinnedRayTracingParseTest, FoxAnimationChangesRaytraceTriangles)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_,
        {.prefer_hardware_raytracing = true});
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto fox_node_id = level.GetIdFromName("FoxMesh");
    ASSERT_NE(fox_node_id, frame::NullId);
    auto& fox_node = level.GetSceneNodeFromId(fox_node_id);
    const auto fox_mesh_id = fox_node.GetLocalMesh();
    ASSERT_NE(fox_mesh_id, frame::NullId);
    auto* skinned_mesh = dynamic_cast<frame::vulkan::SkinnedMesh*>(
        &level.GetMeshFromId(fox_mesh_id));
    ASSERT_NE(skinned_mesh, nullptr);

    const auto triangles_at_start =
        skinned_mesh->EvaluateRaytraceTriangles(skinned_mesh->GetSkinningTime(0.0));
    const auto triangles_during_walk =
        skinned_mesh->EvaluateRaytraceTriangles(skinned_mesh->GetSkinningTime(0.35));
    ASSERT_FALSE(triangles_at_start.empty());
    ASSERT_EQ(triangles_at_start.size(), triangles_during_walk.size());

    bool found_difference = false;
    for (std::size_t i = 0; i < triangles_at_start.size(); i += 97)
    {
        if (std::abs(triangles_at_start[i] - triangles_during_walk[i]) > 1.0e-5f)
        {
            found_difference = true;
            break;
        }
    }
    EXPECT_TRUE(found_difference);
}

TEST_F(VulkanSkinnedRayTracingParseTest, SceneMaterialUsesImportedFoxTextures)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto scene_material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto fox_material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "FoxMesh");
    ASSERT_NE(fox_material_id, frame::NullId);

    const auto& scene_material = level.GetMaterialFromId(scene_material_id);
    const auto& fox_material = level.GetMaterialFromId(fox_material_id);

    const auto scene_albedo_id = FindTextureByInnerName(
        scene_material, "opaque_albedo_texture");
    const auto scene_normal_id = FindTextureByInnerName(
        scene_material, "opaque_normal_texture");
    const auto fox_albedo_id = FindTextureByInnerName(
        fox_material, "albedo_texture");
    const auto fox_normal_id = FindTextureByInnerName(
        fox_material, "normal_texture");

    ASSERT_NE(scene_albedo_id, frame::NullId);
    ASSERT_NE(scene_normal_id, frame::NullId);
    ASSERT_NE(fox_albedo_id, frame::NullId);
    ASSERT_NE(fox_normal_id, frame::NullId);
    EXPECT_EQ(scene_albedo_id, fox_albedo_id);
    EXPECT_EQ(scene_normal_id, fox_normal_id);
    EXPECT_GT(level.GetTextureFromId(fox_albedo_id).GetSize().x, 1u);
    EXPECT_GT(level.GetTextureFromId(fox_albedo_id).GetSize().y, 1u);
}

TEST_F(VulkanSkinnedRayTracingParseTest, QuaternionNodesStillRotate)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto env_holder_id = level.GetIdFromName("env_holder");
    ASSERT_NE(env_holder_id, frame::NullId);
    const auto& env_holder = level.GetSceneNodeFromId(env_holder_id);

    const glm::mat4 model_at_zero = env_holder.GetLocalModel(0.0);
    const glm::mat4 model_at_one = env_holder.GetLocalModel(1.0);

    EXPECT_NEAR(model_at_zero[0][2], 0.0f, 1.0e-5f);
    EXPECT_GT(std::abs(model_at_one[0][2]), 1.0e-4f);
}

TEST_F(
    VulkanSkinnedRayTracingParseTest,
    WorldSpaceRaytraceSceneStateUsesIdentityModelForSkinnedScene)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);

    const auto scene_state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {512u, 288u},
        1.0f,
        material_id,
        false,
        "mesh_holder",
        true);

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            const float expected = row == col ? 1.0f : 0.0f;
            EXPECT_NEAR(scene_state.model[row][col], expected, 1.0e-5f);
        }
    }
    EXPECT_GT(std::abs(scene_state.env_map_model[0][2]), 1.0e-4f);
}

TEST_F(VulkanRayTracingParseTest, BuildsTriangleAndBvhBuffersFromScene)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const std::vector<std::string> expected_buffers = {
        "TriangleBufferTransmissive",
        "BvhBufferTransmissive",
        "TriangleBufferOpaque",
        "BvhBufferOpaque"};
    for (const auto& inner_name : expected_buffers)
    {
        auto id = FindBufferByInnerName(level, material, inner_name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << inner_name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << inner_name;
        if (inner_name.find("Opaque") != std::string::npos)
        {
            EXPECT_GT(buffer->GetSize(), 0u) << "Empty buffer " << inner_name;
        }
    }
}

TEST_F(VulkanRayTracingParseTest, UsesHardwareRaytracingStageFiles)
{
    const auto* program_info = FindProgramInfoByName(
        level_data_, "RayTraceProgram");
    ASSERT_NE(program_info, nullptr);
    EXPECT_EQ(program_info->vulkan.raygen_shader, "raytrace.rgen");
    EXPECT_EQ(program_info->vulkan.miss_shader, "raytrace.rmiss");
    EXPECT_EQ(program_info->vulkan.closesthit_shader, "raytrace.rchit");
}

TEST_F(VulkanRayTracingParseTest, DragonSceneUsesSceneFallbackPbrTextures)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level.GetMaterialFromId(material_id);

    const auto albedo_id = FindTextureByInnerName(
        material, "opaque_albedo_texture");
    const auto normal_id = FindTextureByInnerName(
        material, "opaque_normal_texture");
    const auto roughness_id = FindTextureByInnerName(
        material, "opaque_roughness_texture");
    const auto metallic_id = FindTextureByInnerName(
        material, "opaque_metallic_texture");
    const auto ao_id = FindTextureByInnerName(
        material, "opaque_ao_texture");

    ASSERT_NE(albedo_id, frame::NullId);
    ASSERT_NE(normal_id, frame::NullId);
    ASSERT_NE(roughness_id, frame::NullId);
    ASSERT_NE(metallic_id, frame::NullId);
    ASSERT_NE(ao_id, frame::NullId);

    EXPECT_EQ(albedo_id, level.GetIdFromName("albedo_texture"));
    EXPECT_EQ(normal_id, level.GetIdFromName("normal_texture"));
    EXPECT_EQ(roughness_id, level.GetIdFromName("roughness_texture"));
    EXPECT_EQ(metallic_id, level.GetIdFromName("metallic_texture"));
    EXPECT_EQ(ao_id, level.GetIdFromName("ao_texture"));
}

TEST(VulkanRayTracingTintedMeshTest, ImportedBaseColorTintOverridesColorTexture)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_path = frame::file::FindFile("asset/json/tinted_mesh.json");
    const auto level_proto = frame::json::LoadLevelProto(level_path);
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(512, 288), level_proto, asset_root);

    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto tinted_material_id = FindMaterialForNode(
        level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "TintedTriangle");
    ASSERT_NE(tinted_material_id, frame::NullId);
    const auto& tinted_material = level.GetMaterialFromId(tinted_material_id);

    auto tinted_albedo_id = FindTextureByInnerName(
        tinted_material, "albedo_texture");
    if (tinted_albedo_id == frame::NullId)
    {
        tinted_albedo_id = FindTextureByInnerName(tinted_material, "Color");
    }
    ASSERT_NE(tinted_albedo_id, frame::NullId);

    const auto fallback_color_id = level.GetIdFromName("Color");
    ASSERT_NE(fallback_color_id, frame::NullId);
    EXPECT_NE(tinted_albedo_id, fallback_color_id);

    const auto tinted_albedo = ReadTextureRgb(level, tinted_albedo_id);
    EXPECT_NEAR(tinted_albedo[0], 1.0f, 0.02f);
    EXPECT_NEAR(tinted_albedo[1], 0.0f, 0.02f);
    EXPECT_NEAR(tinted_albedo[2], 0.0f, 0.02f);

    const auto scene_material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto& scene_material = level.GetMaterialFromId(scene_material_id);
    const auto scene_albedo_id = FindTextureByInnerName(
        scene_material, "opaque_albedo_texture");
    ASSERT_NE(scene_albedo_id, frame::NullId);
    EXPECT_EQ(scene_albedo_id, tinted_albedo_id);
}

TEST_F(VulkanRayTracingParseTest, HardwarePreferredBuildSkipsCpuBvhData)
{
    auto built = frame::vulkan::BuildLevel(
        glm::uvec2(512, 288),
        level_data_,
        {.prefer_hardware_raytracing = true});
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    const auto bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);

    auto* tri_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    auto* bvh_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(bvh_id));
    ASSERT_NE(tri_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);

    EXPECT_GT(tri_buffer->GetSize(), 0u);
    EXPECT_EQ(bvh_buffer->GetSize(), 0u);
}

TEST_F(VulkanRayTracingParseTest, TriangleAndBvhDataLooksValid)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    auto bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);

    auto* tri_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    auto* bvh_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(bvh_id));
    ASSERT_NE(tri_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);

    const auto& tri_bytes = tri_buffer->GetRawData();
    const auto& bvh_bytes = bvh_buffer->GetRawData();
    ASSERT_GE(
        tri_bytes.size(), sizeof(float) * kFloatsPerTriangle);
    ASSERT_GE(bvh_bytes.size(), sizeof(BvhNode));

    ASSERT_EQ(tri_bytes.size() % sizeof(float), 0u);
    const std::size_t tri_count =
        tri_bytes.size() / (sizeof(float) * kFloatsPerTriangle);
    const std::size_t node_count = bvh_bytes.size() / sizeof(BvhNode);
    EXPECT_GT(tri_count, 0u);
    EXPECT_GT(node_count, 0u);

    const float* tri_floats =
        reinterpret_cast<const float*>(tri_bytes.data());
    Triangle first_tri = ReadTriangle(tri_floats, 0);
    EXPECT_TRUE(std::isfinite(first_tri.v0.position.x));
    EXPECT_NEAR(glm::length(first_tri.v0.normal), 1.0f, 1.0f);
    EXPECT_GE(first_tri.v0.uv.x, -10.0f);
    EXPECT_LE(first_tri.v0.uv.x, 10.0f);

    BvhNode root{};
    std::memcpy(&root, bvh_bytes.data(), sizeof(BvhNode));
    EXPECT_TRUE(root.triangle_count > 0 || root.left >= 0 || root.right >= 0);
    EXPECT_LT(root.min.x, root.max.x);
}

TEST_F(VulkanRayTracingParseTest, DragonTriangleUvsVaryWhenMeshHasNoTexcoords)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    const auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* tri_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    ASSERT_NE(tri_buffer, nullptr);

    const auto& raw = tri_buffer->GetRawData();
    ASSERT_GE(raw.size(), sizeof(float) * kFloatsPerVertex);
    ASSERT_EQ(raw.size() % sizeof(float), 0u);

    const auto* data = reinterpret_cast<const float*>(raw.data());
    const std::size_t vertex_count = raw.size() / (sizeof(float) * kFloatsPerVertex);
    float min_u = std::numeric_limits<float>::max();
    float max_u = std::numeric_limits<float>::lowest();
    float min_v = std::numeric_limits<float>::max();
    float max_v = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        const std::size_t base = i * kFloatsPerVertex;
        min_u = std::min(min_u, data[base + 8]);
        max_u = std::max(max_u, data[base + 8]);
        min_v = std::min(min_v, data[base + 9]);
        max_v = std::max(max_v, data[base + 9]);
    }

    EXPECT_GT(max_u - min_u, 0.25f);
    EXPECT_GT(max_v - min_v, 0.25f);
}

namespace
{

bool RayAabbIntersect(
    const glm::vec3& ray_origin,
    const glm::vec3& inv_ray_dir,
    const BvhNode& node)
{
    glm::vec3 t0 = (glm::vec3(node.min) - ray_origin) * inv_ray_dir;
    glm::vec3 t1 = (glm::vec3(node.max) - ray_origin) * inv_ray_dir;
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);
    float t_enter = std::max(std::max(tmin.x, tmin.y), tmin.z);
    float t_exit = std::min(std::min(tmax.x, tmax.y), tmax.z);
    return t_exit >= std::max(t_enter, 0.0f);
}

bool RayTriangleIntersect(
    const glm::vec3& ray_origin,
    const glm::vec3& ray_direction,
    const Triangle& tri,
    float& out_t)
{
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 =
        glm::vec3(tri.v1.position) - glm::vec3(tri.v0.position);
    glm::vec3 edge2 =
        glm::vec3(tri.v2.position) - glm::vec3(tri.v0.position);
    glm::vec3 h = glm::cross(ray_direction, edge2);
    float a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;
    float f = 1.0f / a;
    glm::vec3 s = ray_origin - glm::vec3(tri.v0.position);
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray_direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t = f * glm::dot(edge2, q);
    if (t > EPSILON)
    {
        out_t = t;
        return true;
    }
    return false;
}

bool TraverseBvh(
    const std::vector<BvhNode>& nodes,
    const std::vector<Triangle>& tris,
    const glm::vec3& ray_origin,
    const glm::vec3& ray_dir)
{
    glm::vec3 inv_ray_dir = 1.0f / ray_dir;
    int stack[64];
    int sp = 0;
    stack[sp++] = 0;
    while (sp > 0)
    {
        int idx = stack[--sp];
        if (idx < 0 || static_cast<std::size_t>(idx) >= nodes.size())
        {
            continue;
        }
        const auto& node = nodes[idx];
        if (!RayAabbIntersect(ray_origin, inv_ray_dir, node))
        {
            continue;
        }
        if (node.triangle_count > 0)
        {
            for (int i = 0; i < node.triangle_count; ++i)
    {
        int tri_index = node.first_triangle + i;
        if (tri_index < 0 ||
            static_cast<std::size_t>(tri_index) >= tris.size())
        {
            continue;
        }
        float t = 0.0f;
        if (RayTriangleIntersect(
                ray_origin, ray_dir, tris[tri_index], t))
        {
            return true;
        }
    }
        }
        else
        {
            if (node.left >= 0)
            {
                stack[sp++] = node.left;
            }
            if (node.right >= 0)
            {
                stack[sp++] = node.right;
            }
        }
    }
    return false;
}

} // namespace

TEST_F(VulkanRayTracingParseTest, CpuTraversesBvhFromCameraCenterRay)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(1280, 720), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    auto bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);
    auto* tri_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    auto* bvh_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(bvh_id));
    ASSERT_NE(tri_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);

    const auto& tri_bytes = tri_buffer->GetRawData();
    const auto& bvh_bytes = bvh_buffer->GetRawData();
    ASSERT_EQ(tri_bytes.size() % sizeof(float), 0u);
    const std::size_t tri_count =
        tri_bytes.size() / (sizeof(float) * kFloatsPerTriangle);
    const std::size_t node_count = bvh_bytes.size() / sizeof(BvhNode);
    const float* tri_floats =
        reinterpret_cast<const float*>(tri_bytes.data());
    std::vector<Triangle> tris;
    tris.reserve(tri_count);
    for (std::size_t i = 0; i < tri_count; ++i)
    {
        tris.push_back(ReadTriangle(tri_floats, i));
    }
    std::vector<BvhNode> nodes(node_count);
    std::memcpy(nodes.data(), bvh_bytes.data(), bvh_bytes.size());

    // Build a central ray from the default camera toward the origin in model space.
    frame::Camera camera_for_frame(level.GetDefaultCamera());
    glm::vec3 origin = camera_for_frame.GetPosition();
    glm::vec3 dir = glm::normalize(glm::vec3(0.0f) - origin);

    EXPECT_TRUE(TraverseBvh(nodes, tris, origin, dir));
}

TEST_F(VulkanRayTracingParseTest, ShaderLikeRayFromCenterHitsGeometry)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(1280, 720), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    auto triangle_id = FindBufferByInnerName(
        level, material, "TriangleBufferOpaque");
    auto bvh_id = FindBufferByInnerName(
        level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);
    auto* tri_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(triangle_id));
    auto* bvh_buffer =
        dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(bvh_id));
    ASSERT_NE(tri_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);

    const auto& tri_bytes = tri_buffer->GetRawData();
    const auto& bvh_bytes = bvh_buffer->GetRawData();
    ASSERT_EQ(tri_bytes.size() % sizeof(float), 0u);
    const std::size_t tri_count =
        tri_bytes.size() / (sizeof(float) * kFloatsPerTriangle);
    const std::size_t node_count = bvh_bytes.size() / sizeof(BvhNode);
    const float* tri_floats =
        reinterpret_cast<const float*>(tri_bytes.data());
    std::vector<Triangle> tris;
    tris.reserve(tri_count);
    for (std::size_t i = 0; i < tri_count; ++i)
    {
        tris.push_back(ReadTriangle(tri_floats, i));
    }
    std::vector<BvhNode> nodes(node_count);
    std::memcpy(nodes.data(), bvh_bytes.data(), bvh_bytes.size());

    // Build the same matrices the compute shader uses.
    auto scene_state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {1280u, 720u},
        0.0f,
        level.GetIdFromName("RayTraceMaterial"),
        false);

    glm::mat4 proj_inv = glm::inverse(scene_state.projection);
    glm::mat4 view_inv = glm::inverse(scene_state.view);
    glm::mat4 model_inv = glm::inverse(scene_state.model);

    // Center pixel ray as in shader.
    glm::vec2 uv = glm::vec2(0.5f);
    glm::vec2 ndc = uv * 2.0f - 1.0f;
    glm::vec4 clip_pos(ndc, -1.0f, 1.0f);
    glm::vec4 view_pos = proj_inv * clip_pos;
    view_pos = glm::vec4(view_pos.x, view_pos.y, -1.0f, 0.0f);
    glm::vec3 ray_dir_world = glm::normalize(glm::vec3(view_inv * view_pos));

    glm::vec3 ray_origin = glm::vec3(model_inv * glm::vec4(
        scene_state.camera_position, 1.0f));
    glm::vec3 ray_dir = glm::normalize(glm::mat3(model_inv) * ray_dir_world);

    // Ensure the ray intersects the root AABB.
    ASSERT_TRUE(RayAabbIntersect(ray_origin, 1.0f / ray_dir, nodes.front()));
    EXPECT_TRUE(TraverseBvh(nodes, tris, ray_origin, ray_dir));
}
TEST_F(VulkanRayTracingParseTest, BuildsSceneStateWithoutExplicitModelNodeBinding)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);
    EXPECT_TRUE(material.GetNodeNames().empty());

    auto scene_state = frame::vulkan::BuildSceneState(
        level,
        frame::Logger::GetInstance(),
        {512u, 288u},
        0.0f,
        material_id,
        false);
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_TRUE(std::isfinite(scene_state.model[row][col]));
        }
    }
}

} // namespace test
