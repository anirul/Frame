#pragma once

#include "frame/backend_internal.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "frame/entity_id.h"
#include "frame/vulkan/vulkan_dispatch.h"

namespace frame
{
class LevelInterface;
}

namespace frame::vulkan
{

inline constexpr std::size_t kRaytraceFloatsPerVertex = 12;
inline constexpr std::size_t kRaytraceTriangleVertexStrideBytes =
    sizeof(float) * kRaytraceFloatsPerVertex;
inline constexpr std::uint32_t kGpuSkinningBoneCapacity = 128u;
inline constexpr std::uint32_t kGpuSkinningWorkgroupSize = 64u;

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

struct alignas(16) GpuSkinningPushConstants
{
    std::uint32_t output_vertex_count = 0;
    std::uint32_t pad0 = 0;
    std::uint32_t pad1 = 0;
    std::uint32_t pad2 = 0;
    glm::vec4 color_multiplier = glm::vec4(1.0f);
    glm::vec4 atlas_uv_bounds = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
};

struct alignas(16) HardwareRaytraceInstanceStorageData
{
    glm::mat4 object_to_world = glm::mat4(1.0f);
    glm::mat4 world_to_object = glm::mat4(1.0f);
    glm::uvec4 metadata = glm::uvec4(0u);
};

struct RaytracingSourceGeometryData
{
    EntityId source_node_id = NullId;
    EntityId triangle_buffer_id = NullId;
    EntityId source_material_id = NullId;
    std::uint32_t material_id = 0;
    std::uint32_t triangle_offset = 0;
    std::array<float, 4> atlas_uv_bounds = {0.0f, 1.0f, 0.0f, 1.0f};
    std::vector<std::uint8_t> triangle_bytes = {};
};

std::array<float, 4> ResolveRaytracingSourceMaterialColor(
    LevelInterface& level, EntityId material_id);
std::array<float, 4> ResolveRaytracingReferenceColor(
    LevelInterface& level, bool transmissive);
std::array<float, 4> ResolveRaytracingColorMultiplier(
    const std::array<float, 4>& source_color,
    const std::array<float, 4>& reference_color);

std::size_t BuildRaytracingSourceStateHash(
    LevelInterface& level, double time_seconds, bool include_node_matrices);
bool RaytraceSceneRequiresWorldSpaceBuffers(LevelInterface& level);
bool HasRaytracingSourceMeshes(LevelInterface& level);
std::optional<glm::mat4> GetSharedRaytraceSceneTransform(
    LevelInterface& level, double time_seconds);
bool CanUseSharedTransformHardwareRaytraceScene(
    LevelInterface& level, double time_seconds);

inline std::uint32_t GetAccelerationStructureMaxVertex(
    std::uint32_t vertex_count)
{
    return vertex_count > 0u ? vertex_count - 1u : 0u;
}
std::vector<std::uint8_t> BuildGpuSkinningSourceVertexBytes(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::int32_t>& bone_indices,
    const std::vector<float>& bone_weights);
std::vector<std::uint8_t> BuildFlatBytes(
    const std::vector<std::uint32_t>& values);
inline vk::TransformMatrixKHR MakeIdentityTransform()
{
    vk::TransformMatrixKHR transform = {};
    transform.matrix[0][0] = 1.0f;
    transform.matrix[1][1] = 1.0f;
    transform.matrix[2][2] = 1.0f;
    return transform;
}

inline glm::mat4 InverseOrIdentity(const glm::mat4& matrix)
{
    const float determinant = glm::determinant(glm::mat3(matrix));
    if (std::abs(determinant) <= 1.0e-8f)
    {
        return glm::mat4(1.0f);
    }
    return glm::inverse(matrix);
}

inline vk::TransformMatrixKHR MakeAccelerationStructureTransform(
    const glm::mat4& matrix)
{
    vk::TransformMatrixKHR transform = {};
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            transform.matrix[row][column] = matrix[column][row];
        }
    }
    return transform;
}

std::vector<RaytracingSourceGeometryData> BuildRaytracingSourceGeometryData(
    LevelInterface& level, double time_seconds, bool apply_node_transform);
std::optional<RaytracingSourceGeometryData>
BuildRaytracingSourceGeometryDataForSource(
    LevelInterface& level,
    EntityId source_node_id,
    EntityId source_material_id,
    double time_seconds,
    bool apply_node_transform,
    std::uint32_t triangle_offset);
const RaytracingSourceGeometryData* FindPreparedRaytracingSourceGeometry(
    const std::vector<RaytracingSourceGeometryData>& prepared_source_geometries,
    EntityId source_node_id,
    EntityId source_material_id,
    std::uint32_t triangle_offset);
std::vector<RaytracingSourceGeometryData>
BuildPreparedUpdatedRaytracingSourceGeometries(
    LevelInterface& level,
    const std::vector<EntityId>& updated_source_triangle_buffer_ids,
    double time_seconds);

std::vector<std::uint8_t> BuildSequentialIndexBytes(std::uint32_t vertex_count);
std::vector<std::uint8_t> BuildAggregateTriangleBytes(
    LevelInterface& level,
    bool transmissive,
    double time_seconds,
    bool apply_node_transform = true);
std::vector<std::uint8_t> BuildAggregateBvhBytes(
    const std::vector<std::uint8_t>& triangle_bytes);

} // namespace frame::vulkan
