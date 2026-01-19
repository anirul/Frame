#include <algorithm>
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
        level_path_ = frame::file::FindFile("asset/json/raytracing_bvh.json");
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

TEST_F(VulkanRayTracingDualParseTest, BuildsTriangleBuffersFromScene)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    bool found_triangle_buffer = false;
    for (const auto& name : material.GetBufferNames())
    {
        if (!EndsWith(name, ".triangle"))
        {
            continue;
        }
        found_triangle_buffer = true;
        auto id = level.GetIdFromName(name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << name;
        EXPECT_GT(buffer->GetSize(), 0u) << "Empty buffer " << name;
    }
    EXPECT_TRUE(found_triangle_buffer);
}

TEST_F(VulkanRayTracingDualParseTest, TriangleDataLooksValid)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    for (const auto& name : material.GetBufferNames())
    {
        if (!EndsWith(name, ".triangle"))
        {
            continue;
        }
        auto id = level.GetIdFromName(name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << name;
        ExpectTriangleBufferValid(*buffer);
    }
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
        "AppleMesh.0.triangle",
        "AppleMesh.0.bvh"};
    for (const auto& name : expected_buffers)
    {
        auto names = material.GetBufferNames();
        EXPECT_NE(
            std::find(names.begin(), names.end(), name), names.end())
            << "Material should expose buffer " << name;
        auto id = level.GetIdFromName(name);
        ASSERT_NE(id, frame::NullId) << "Missing buffer " << name;
        auto* buffer =
            dynamic_cast<frame::vulkan::Buffer*>(&level.GetBufferFromId(id));
        ASSERT_NE(buffer, nullptr) << "Unexpected buffer type for " << name;
        EXPECT_GT(buffer->GetSize(), 0u) << "Empty buffer " << name;
    }
}

TEST_F(VulkanRayTracingParseTest, TriangleAndBvhDataLooksValid)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    auto triangle_id = level.GetIdFromName("AppleMesh.0.triangle");
    auto bvh_id = level.GetIdFromName("AppleMesh.0.bvh");
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

    auto triangle_id = level.GetIdFromName("AppleMesh.0.triangle");
    auto bvh_id = level.GetIdFromName("AppleMesh.0.bvh");
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

    auto triangle_id = level.GetIdFromName("AppleMesh.0.triangle");
    auto bvh_id = level.GetIdFromName("AppleMesh.0.bvh");
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
TEST_F(VulkanRayTracingParseTest, MapsRayTracingNodesForUniforms)
{
    auto built = frame::vulkan::BuildLevel(glm::uvec2(512, 288), level_data_);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;

    const auto material_id = level.GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    auto& material = level.GetMaterialFromId(material_id);

    auto node_names = material.GetNodeNames();
    EXPECT_NE(
        std::find(node_names.begin(), node_names.end(), "AppleMesh"),
        node_names.end());

    const auto node_id = level.GetIdFromName("AppleMesh");
    ASSERT_NE(node_id, frame::NullId);
    EXPECT_EQ(material.GetInnerNodeName("AppleMesh"), "model");
    EXPECT_NO_THROW(level.GetSceneNodeFromId(node_id));
}

} // namespace test
