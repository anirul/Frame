#include "frame/opengl/raytracing_test.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "frame/camera.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/texture.h"

namespace test
{

namespace
{

struct RaytraceVertex
{
    float px;
    float py;
    float pz;
    float pad0;
    float nx;
    float ny;
    float nz;
    float pad1;
    float u;
    float v;
    float pad2;
    float pad3;
};

struct PixelRect
{
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
    bool valid = false;
};

bool RayTriangleIntersect(
    const glm::vec3& ray_origin,
    const glm::vec3& ray_direction,
    const RaytraceVertex& v0,
    const RaytraceVertex& v1,
    const RaytraceVertex& v2)
{
    constexpr float kEpsilon = 1.0e-7f;
    const glm::vec3 p0(v0.px, v0.py, v0.pz);
    const glm::vec3 p1(v1.px, v1.py, v1.pz);
    const glm::vec3 p2(v2.px, v2.py, v2.pz);
    const glm::vec3 edge1 = p1 - p0;
    const glm::vec3 edge2 = p2 - p0;
    const glm::vec3 h = glm::cross(ray_direction, edge2);
    const float a = glm::dot(edge1, h);
    if (a > -kEpsilon && a < kEpsilon)
    {
        return false;
    }
    const float f = 1.0f / a;
    const glm::vec3 s = ray_origin - p0;
    const float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }
    const glm::vec3 q = glm::cross(s, edge1);
    const float v = f * glm::dot(ray_direction, q);
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }
    const float t = f * glm::dot(edge2, q);
    return t > kEpsilon;
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

std::vector<std::uint8_t> RenderOutputBytes(
    frame::LevelInterface& level, double time_seconds = 0.0)
{
    frame::opengl::Renderer renderer(
        level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDepthTest(true);
    renderer.SetDeltaTime(time_seconds);

    frame::Camera camera(level.GetDefaultCamera());
    camera.SetAspectRatio(1280.0f / 720.0f);
    renderer.PreRender();
    renderer.RenderSkybox(camera);
    renderer.RenderScene(camera);
    renderer.PostProcess();

    const auto output_texture_id = level.GetDefaultOutputTextureId();
    if (output_texture_id == frame::NullId)
    {
        throw std::runtime_error("No default output texture id.");
    }

    auto* output_texture = dynamic_cast<frame::opengl::Texture*>(
        &level.GetTextureFromId(output_texture_id));
    if (!output_texture)
    {
        throw std::runtime_error("Default output texture is not an OpenGL texture.");
    }
    if (output_texture->GetData().pixel_element_size().value() !=
        frame::proto::PixelElementSize::BYTE)
    {
        throw std::runtime_error("Expected byte output texture for OpenGL render test.");
    }
    return output_texture->GetTextureByte();
}

double ComputeAverageNormalizedDifference(
    const std::vector<std::uint8_t>& lhs,
    const std::vector<std::uint8_t>& rhs)
{
    const std::size_t size = std::min(lhs.size(), rhs.size());
    if (size == 0)
    {
        return 0.0;
    }

    double accum = 0.0;
    for (std::size_t i = 0; i < size; ++i)
    {
        accum += std::abs(
            static_cast<int>(lhs[i]) - static_cast<int>(rhs[i]));
    }
    return accum / (static_cast<double>(size) * 255.0);
}

double ComputeAverageNormalizedDifferenceInRect(
    const std::vector<std::uint8_t>& lhs,
    const std::vector<std::uint8_t>& rhs,
    int width,
    int height,
    const PixelRect& rect)
{
    if (!rect.valid || width <= 0 || height <= 0)
    {
        return 0.0;
    }

    const int min_x = std::clamp(rect.min_x, 0, width - 1);
    const int min_y = std::clamp(rect.min_y, 0, height - 1);
    const int max_x = std::clamp(rect.max_x, 0, width - 1);
    const int max_y = std::clamp(rect.max_y, 0, height - 1);
    if (min_x > max_x || min_y > max_y)
    {
        return 0.0;
    }

    double accum = 0.0;
    std::size_t count = 0;
    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            const std::size_t base =
                (static_cast<std::size_t>(y) * width + x) * 4;
            if (base + 2 >= lhs.size() || base + 2 >= rhs.size())
            {
                continue;
            }
            accum += std::abs(
                static_cast<int>(lhs[base + 0]) -
                static_cast<int>(rhs[base + 0]));
            accum += std::abs(
                static_cast<int>(lhs[base + 1]) -
                static_cast<int>(rhs[base + 1]));
            accum += std::abs(
                static_cast<int>(lhs[base + 2]) -
                static_cast<int>(rhs[base + 2]));
            count += 3;
        }
    }
    return count == 0 ? 0.0 : accum / (static_cast<double>(count) * 255.0);
}

PixelRect ComputeProjectedPixelRect(
    frame::LevelInterface& level,
    const std::vector<std::uint8_t>& triangle_bytes,
    int width,
    int height)
{
    PixelRect rect = {};
    if (triangle_bytes.size() < sizeof(RaytraceVertex))
    {
        return rect;
    }

    frame::Camera camera(level.GetDefaultCamera());
    camera.SetAspectRatio(static_cast<float>(width) / height);
    const glm::mat4 projection = camera.ComputeProjection();
    const glm::mat4 view = camera.ComputeView();

    const auto* vertices =
        reinterpret_cast<const RaytraceVertex*>(triangle_bytes.data());
    const std::size_t vertex_count =
        triangle_bytes.size() / sizeof(RaytraceVertex);

    float min_x = static_cast<float>(width);
    float min_y = static_cast<float>(height);
    float max_x = 0.0f;
    float max_y = 0.0f;
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        const glm::vec4 clip = projection * view *
            glm::vec4(vertices[i].px, vertices[i].py, vertices[i].pz, 1.0f);
        if (clip.w <= 1.0e-6f)
        {
            continue;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (std::abs(ndc.x) > 1.2f || std::abs(ndc.y) > 1.2f)
        {
            continue;
        }
        const float px =
            (ndc.x * 0.5f + 0.5f) * static_cast<float>(width - 1);
        const float py =
            (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(height - 1);
        min_x = std::min(min_x, px);
        min_y = std::min(min_y, py);
        max_x = std::max(max_x, px);
        max_y = std::max(max_y, py);
        rect.valid = true;
    }

    if (!rect.valid)
    {
        return rect;
    }

    rect.min_x = static_cast<int>(std::floor(min_x));
    rect.min_y = static_cast<int>(std::floor(min_y));
    rect.max_x = static_cast<int>(std::ceil(max_x));
    rect.max_y = static_cast<int>(std::ceil(max_y));
    return rect;
}

bool HasInnerBuffer(
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto& buffer_name : material.GetBufferNames())
    {
        if (material.GetInnerBufferName(buffer_name) == expected_inner_name)
        {
            return true;
        }
    }
    return false;
}

frame::EntityId FindBufferByInnerName(
    const frame::LevelInterface& level,
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto& buffer_name : material.GetBufferNames())
    {
        if (material.GetInnerBufferName(buffer_name) != expected_inner_name)
        {
            continue;
        }
        return level.GetIdFromName(buffer_name);
    }
    return frame::NullId;
}

} // namespace

TEST_F(OpenGLRayTracingLevelTest, RaytracingLevelBindsSceneTriangleBuffers)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferTransmissive"));
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferOpaque"));
}

TEST_F(
    OpenGLRayTracingLevelTest,
    StaticRaytracingSceneKeepsAggregateBuffersStableAcrossFrames)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);

    const auto opaque_triangle_id = FindBufferByInnerName(
        *level, material, "TriangleBufferOpaque");
    const auto transmissive_triangle_id = FindBufferByInnerName(
        *level, material, "TriangleBufferTransmissive");
    ASSERT_NE(opaque_triangle_id, frame::NullId);
    ASSERT_NE(transmissive_triangle_id, frame::NullId);

    auto* opaque_triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(opaque_triangle_id));
    auto* transmissive_triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(transmissive_triangle_id));
    ASSERT_NE(opaque_triangle_buffer, nullptr);
    ASSERT_NE(transmissive_triangle_buffer, nullptr);

    frame::opengl::Renderer renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDeltaTime(0.0);
    renderer.PreRender();
    const auto opaque_triangles_at_zero =
        opaque_triangle_buffer->GetRawData();
    const auto transmissive_triangles_at_zero =
        transmissive_triangle_buffer->GetRawData();

    renderer.SetDeltaTime(1.0);
    renderer.PreRender();

    EXPECT_EQ(opaque_triangle_buffer->GetRawData(), opaque_triangles_at_zero);
    EXPECT_EQ(
        transmissive_triangle_buffer->GetRawData(),
        transmissive_triangles_at_zero);
}

TEST_F(OpenGLRayTracingLevelTest, StaticRaytracingSceneRenderChangesOverTime)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto frame_at_zero = RenderOutputBytes(*level, 0.0);
    const auto frame_at_one = RenderOutputBytes(*level, 1.0);
    ASSERT_FALSE(frame_at_zero.empty());
    ASSERT_EQ(frame_at_zero.size(), frame_at_one.size());

    EXPECT_GT(
        ComputeAverageNormalizedDifference(frame_at_zero, frame_at_one),
        0.005);
}

TEST_F(OpenGLRayTracingLevelTest, DragonLevelBindsAggregateSceneBuffers)
{
    auto level = LoadLevel("asset/json/dragon.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferTransmissive"));
    EXPECT_TRUE(HasInnerBuffer(material, "BvhBufferTransmissive"));
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferOpaque"));
    EXPECT_TRUE(HasInnerBuffer(material, "BvhBufferOpaque"));
}

TEST_F(OpenGLRayTracingLevelTest, DragonLevelUsesSceneFallbackPbrTextures)
{
    auto level = LoadLevel("asset/json/dragon.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);

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

    EXPECT_EQ(albedo_id, level->GetIdFromName("albedo_texture"));
    EXPECT_EQ(normal_id, level->GetIdFromName("normal_texture"));
    EXPECT_EQ(roughness_id, level->GetIdFromName("roughness_texture"));
    EXPECT_EQ(metallic_id, level->GetIdFromName("metallic_texture"));
    EXPECT_EQ(ao_id, level->GetIdFromName("ao_texture"));
}

TEST_F(OpenGLRayTracingLevelTest, DragonLevelGeneratesUvCoordinatesForUnwrappedMesh)
{
    auto level = LoadLevel("asset/json/dragon.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);
    const auto triangle_id = FindBufferByInnerName(
        *level, material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);

    const auto& raw = triangle_buffer->GetRawData();
    ASSERT_GE(raw.size(), sizeof(RaytraceVertex));
    ASSERT_EQ(raw.size() % sizeof(RaytraceVertex), 0u);

    const auto* vertices =
        reinterpret_cast<const RaytraceVertex*>(raw.data());
    const std::size_t vertex_count = raw.size() / sizeof(RaytraceVertex);
    float min_u = std::numeric_limits<float>::max();
    float max_u = std::numeric_limits<float>::lowest();
    float min_v = std::numeric_limits<float>::max();
    float max_v = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        min_u = std::min(min_u, vertices[i].u);
        max_u = std::max(max_u, vertices[i].u);
        min_v = std::min(min_v, vertices[i].v);
        max_v = std::max(max_v, vertices[i].v);
    }

    EXPECT_GT(max_u - min_u, 0.25f);
    EXPECT_GT(max_v - min_v, 0.25f);
}

TEST_F(OpenGLRayTracingLevelTest, ImportsGltfGlassMaterialFromIco)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::PRE_RENDER_TIME,
        "Ico");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);

    const auto transmission_texture_id = FindTextureByInnerName(
        material, "transmission_texture");
    const auto ior_texture_id = FindTextureByInnerName(material, "ior_texture");
    const auto thickness_texture_id = FindTextureByInnerName(
        material, "thickness_texture");
    const auto attenuation_color_texture_id = FindTextureByInnerName(
        material, "attenuation_color_texture");
    const auto attenuation_distance_texture_id = FindTextureByInnerName(
        material, "attenuation_distance_texture");

    ASSERT_NE(transmission_texture_id, frame::NullId);
    ASSERT_NE(ior_texture_id, frame::NullId);
    ASSERT_NE(thickness_texture_id, frame::NullId);
    ASSERT_NE(attenuation_color_texture_id, frame::NullId);
    ASSERT_NE(attenuation_distance_texture_id, frame::NullId);

    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, transmission_texture_id),
        1.0f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, ior_texture_id),
        1.45f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, thickness_texture_id),
        0.35f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, attenuation_distance_texture_id),
        1.5f,
        0.02f);

    const auto attenuation_color = ReadTextureRgb(
        *level, attenuation_color_texture_id);
    EXPECT_NEAR(attenuation_color[0], 0.82f, 0.03f);
    EXPECT_NEAR(attenuation_color[1], 0.97f, 0.03f);
    EXPECT_NEAR(attenuation_color[2], 0.90f, 0.03f);
}

TEST_F(OpenGLRayTracingLevelTest, ImportsGltfOpaqueSpecularFromPlate)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto plate_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::PRE_RENDER_TIME,
        "Plate");
    ASSERT_NE(plate_material_id, frame::NullId);
    const auto scene_material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(scene_material_id, frame::NullId);

    const auto& plate_material = level->GetMaterialFromId(plate_material_id);
    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto plate_specular_factor_id = FindTextureByInnerName(
        plate_material, "specular_factor_texture");
    const auto plate_specular_color_id = FindTextureByInnerName(
        plate_material, "specular_color_texture");
    const auto scene_specular_factor_id = FindTextureByInnerName(
        scene_material, "opaque_specular_factor_texture");
    const auto scene_specular_color_id = FindTextureByInnerName(
        scene_material, "opaque_specular_color_texture");

    ASSERT_NE(plate_specular_factor_id, frame::NullId);
    ASSERT_NE(plate_specular_color_id, frame::NullId);
    ASSERT_NE(scene_specular_factor_id, frame::NullId);
    ASSERT_NE(scene_specular_color_id, frame::NullId);
    EXPECT_EQ(scene_specular_factor_id, plate_specular_factor_id);
    EXPECT_EQ(scene_specular_color_id, plate_specular_color_id);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, plate_specular_factor_id),
        1.0f,
        0.02f);

    const auto specular_color = ReadTextureRgb(*level, plate_specular_color_id);
    EXPECT_NEAR(specular_color[0], 0.0f, 0.02f);
    EXPECT_NEAR(specular_color[1], 0.0f, 0.02f);
    EXPECT_NEAR(specular_color[2], 0.0f, 0.02f);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshPreRenderUpdatesAggregateSceneBuffers)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);

    const auto triangle_id = FindBufferByInnerName(
        *level, material, "TriangleBufferOpaque");
    const auto bvh_id = FindBufferByInnerName(
        *level, material, "BvhBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    ASSERT_NE(bvh_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    auto* bvh_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(bvh_id));
    ASSERT_NE(triangle_buffer, nullptr);
    ASSERT_NE(bvh_buffer, nullptr);

    const auto triangles_before = triangle_buffer->GetRawData();
    const auto bvh_before = bvh_buffer->GetRawData();

    frame::opengl::Renderer renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDeltaTime(0.35);
    renderer.PreRender();

    EXPECT_NE(triangle_buffer->GetRawData(), triangles_before);
    EXPECT_NE(bvh_buffer->GetRawData(), bvh_before);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshSceneMaterialUsesImportedFoxTextures)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    const auto scene_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "RayTracingRendering");
    ASSERT_NE(scene_material_id, frame::NullId);
    EXPECT_EQ(scene_material_id, level->GetIdFromName("RayTraceMaterial"));
    const auto fox_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::PRE_RENDER_TIME,
        "FoxMesh");
    ASSERT_NE(fox_material_id, frame::NullId);

    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto& fox_material = level->GetMaterialFromId(fox_material_id);

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
    EXPECT_GT(level->GetTextureFromId(fox_albedo_id).GetSize().x, 1u);
    EXPECT_GT(level->GetTextureFromId(fox_albedo_id).GetSize().y, 1u);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshFoxMaterialRemainsOpaque)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    const auto fox_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::PRE_RENDER_TIME,
        "FoxMesh");
    ASSERT_NE(fox_material_id, frame::NullId);

    const auto& fox_material = level->GetMaterialFromId(fox_material_id);
    const auto transmission_texture_id = FindTextureByInnerName(
        fox_material, "transmission_texture");
    ASSERT_NE(transmission_texture_id, frame::NullId);
    EXPECT_LT(ReadTextureFirstChannel(*level, transmission_texture_id), 0.01f);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshQuaternionNodesStillRotate)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    const auto env_holder_id = level->GetIdFromName("env_holder");
    ASSERT_NE(env_holder_id, frame::NullId);
    const auto& env_holder = level->GetSceneNodeFromId(env_holder_id);

    const glm::mat4 model_at_zero = env_holder.GetLocalModel(0.0);
    const glm::mat4 model_at_one = env_holder.GetLocalModel(1.0);

    EXPECT_NEAR(model_at_zero[0][2], 0.0f, 1.0e-5f);
    EXPECT_GT(std::abs(model_at_one[0][2]), 1.0e-4f);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshAggregateTrianglesCarryUvs)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    frame::opengl::Renderer renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDeltaTime(0.0);
    renderer.PreRender();

    const auto scene_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "RayTracingRendering");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto triangle_id = FindBufferByInnerName(
        *level, scene_material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);
    const auto& raw = triangle_buffer->GetRawData();
    ASSERT_GE(raw.size(), sizeof(RaytraceVertex));
    ASSERT_EQ(raw.size() % sizeof(RaytraceVertex), 0u);

    const auto* vertices =
        reinterpret_cast<const RaytraceVertex*>(raw.data());
    const std::size_t vertex_count = raw.size() / sizeof(RaytraceVertex);
    float min_u = std::numeric_limits<float>::max();
    float max_u = std::numeric_limits<float>::lowest();
    float min_v = std::numeric_limits<float>::max();
    float max_v = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        min_u = std::min(min_u, vertices[i].u);
        max_u = std::max(max_u, vertices[i].u);
        min_v = std::min(min_v, vertices[i].v);
        max_v = std::max(max_v, vertices[i].v);
    }
    EXPECT_GT(max_u - min_u, 0.25f);
    EXPECT_GT(max_v - min_v, 0.25f);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshAggregateTrianglesHaveFiniteBounds)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    frame::opengl::Renderer renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDeltaTime(0.0);
    renderer.PreRender();

    const auto scene_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "RayTracingRendering");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto triangle_id = FindBufferByInnerName(
        *level, scene_material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);
    const auto& raw = triangle_buffer->GetRawData();
    ASSERT_GE(raw.size(), sizeof(RaytraceVertex));
    ASSERT_EQ(raw.size() % sizeof(RaytraceVertex), 0u);

    const auto* vertices =
        reinterpret_cast<const RaytraceVertex*>(raw.data());
    const std::size_t vertex_count = raw.size() / sizeof(RaytraceVertex);
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    float max_z = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        ASSERT_TRUE(std::isfinite(vertices[i].px));
        ASSERT_TRUE(std::isfinite(vertices[i].py));
        ASSERT_TRUE(std::isfinite(vertices[i].pz));
        min_x = std::min(min_x, vertices[i].px);
        min_y = std::min(min_y, vertices[i].py);
        min_z = std::min(min_z, vertices[i].pz);
        max_x = std::max(max_x, vertices[i].px);
        max_y = std::max(max_y, vertices[i].py);
        max_z = std::max(max_z, vertices[i].pz);
    }

    EXPECT_GT(max_x - min_x, 0.05f);
    EXPECT_GT(max_y - min_y, 0.05f);
    EXPECT_GT(max_z - min_z, 0.05f);
    EXPECT_LT(min_y, 0.2f);
    EXPECT_GT(max_y, 0.2f);
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshCpuRayHitsAggregateTriangles)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    frame::opengl::Renderer renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    renderer.SetDeltaTime(0.0);
    renderer.PreRender();

    const auto scene_material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::SCENE_RENDER_TIME,
        "RayTracingRendering");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto triangle_id = FindBufferByInnerName(
        *level, scene_material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);

    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);
    const auto& raw = triangle_buffer->GetRawData();
    ASSERT_GE(raw.size(), sizeof(RaytraceVertex) * 3);
    ASSERT_EQ(raw.size() % sizeof(RaytraceVertex), 0u);

    const auto* vertices =
        reinterpret_cast<const RaytraceVertex*>(raw.data());
    const std::size_t vertex_count = raw.size() / sizeof(RaytraceVertex);

    frame::Camera camera(level->GetDefaultCamera());
    camera.SetAspectRatio(1280.0f / 720.0f);
    const glm::mat4 projection = camera.ComputeProjection();
    const glm::mat4 view = camera.ComputeView();
    const glm::mat4 projection_inv = glm::inverse(camera.ComputeProjection());
    const glm::mat4 view_inv = glm::inverse(camera.ComputeView());
    const glm::vec3 camera_position = camera.GetPosition();

    std::size_t dense_hit_count = 0;
    for (int y = 0; y < 72; ++y)
    {
        for (int x = 0; x < 128; ++x)
        {
            const glm::vec2 ndc(
                ((static_cast<float>(x) + 0.5f) / 128.0f) * 2.0f - 1.0f,
                ((static_cast<float>(y) + 0.5f) / 72.0f) * 2.0f - 1.0f);
            glm::vec4 clip_pos(ndc.x, ndc.y, 1.0f, 1.0f);
            glm::vec4 view_pos = projection_inv * clip_pos;
            const float view_w =
                std::abs(view_pos.w) > 1.0e-6f ? view_pos.w : 1.0f;
            const glm::vec3 ray_dir_view =
                glm::normalize(glm::vec3(view_pos) / view_w);
            const glm::vec3 ray_dir_world = glm::normalize(
                glm::vec3(view_inv * glm::vec4(ray_dir_view, 0.0f)));
            const glm::vec3 ray_origin = camera_position;
            const glm::vec3 ray_dir = ray_dir_world;
            for (std::size_t i = 0; i + 2 < vertex_count; i += 3)
            {
                if (RayTriangleIntersect(
                        ray_origin,
                        ray_dir,
                        vertices[i],
                        vertices[i + 1],
                        vertices[i + 2]))
                {
                    ++dense_hit_count;
                    break;
                }
            }
        }
    }
    EXPECT_GT(dense_hit_count, 0u);
}

TEST_F(OpenGLRayTracingLevelTest, DISABLED_DumpSkinnedMeshDeviceFrame)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);
    ASSERT_NE(window_, nullptr);

    auto& device = window_->GetDevice();
    device.Startup(std::move(level));
    device.Display(0.0);
    device.ScreenShot("build/windows/skinned_mesh_opengl_debug.png");
}

TEST_F(OpenGLRayTracingLevelTest, SkinnedMeshRenderUsesFoxAlbedoTexture)
{
    auto level = LoadLevel("asset/json/skinned_mesh.json");
    ASSERT_NE(level, nullptr);

    frame::opengl::Renderer aggregate_renderer(
        *level,
        glm::uvec4(0, 0, 1280, 720));
    aggregate_renderer.SetDeltaTime(0.0);
    aggregate_renderer.PreRender();

    const auto scene_material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(scene_material_id, frame::NullId);
    const auto& scene_material = level->GetMaterialFromId(scene_material_id);
    const auto triangle_id = FindBufferByInnerName(
        *level, scene_material, "TriangleBufferOpaque");
    ASSERT_NE(triangle_id, frame::NullId);
    auto* triangle_buffer = dynamic_cast<frame::opengl::Buffer*>(
        &level->GetBufferFromId(triangle_id));
    ASSERT_NE(triangle_buffer, nullptr);

    const auto albedo_texture_id = FindTextureByInnerName(
        scene_material, "opaque_albedo_texture");
    ASSERT_NE(albedo_texture_id, frame::NullId);

    auto baseline = RenderOutputBytes(*level, 0.0);
    ASSERT_FALSE(baseline.empty());

    auto* albedo_texture = dynamic_cast<frame::opengl::Texture*>(
        &level->GetTextureFromId(albedo_texture_id));
    ASSERT_NE(albedo_texture, nullptr);
    const auto albedo_before = albedo_texture->GetTextureByte();
    albedo_texture->Clear(glm::vec4(1.0f));
    const auto albedo_after = albedo_texture->GetTextureByte();
    EXPECT_GT(
        ComputeAverageNormalizedDifference(albedo_before, albedo_after),
        0.05);

    auto white_albedo = RenderOutputBytes(*level, 0.0);
    ASSERT_FALSE(white_albedo.empty());

    const auto rect = ComputeProjectedPixelRect(
        *level, triangle_buffer->GetRawData(), 1280, 720);
    ASSERT_TRUE(rect.valid);
    const double average_difference =
        ComputeAverageNormalizedDifferenceInRect(
            baseline, white_albedo, 1280, 720, rect);
    EXPECT_GT(average_difference, 0.03);
}

} // namespace test
