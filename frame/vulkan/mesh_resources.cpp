#include "frame/vulkan/mesh_resources.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include "frame/vulkan/buffer.h"
#include "frame/vulkan/skinned_mesh.h"

namespace frame::vulkan
{

namespace
{

template <typename T>
std::vector<T> ReadVectorFromBuffer(const frame::BufferInterface& buffer)
{
    const auto* vk_buffer = dynamic_cast<const Buffer*>(&buffer);
    if (!vk_buffer)
    {
        throw std::runtime_error("Expected a Vulkan CPU-backed buffer.");
    }
    const auto& bytes = vk_buffer->GetRawData();
    if (bytes.size() % sizeof(T) != 0)
    {
        throw std::runtime_error("Buffer byte size is not element aligned.");
    }
    std::vector<T> values(bytes.size() / sizeof(T));
    if (!values.empty())
    {
        std::memcpy(values.data(), bytes.data(), bytes.size());
    }
    return values;
}

bool EvaluateSkinnedRasterMesh(
    const frame::vulkan::SkinnedMesh& skinned_mesh,
    double skinning_time_s,
    std::vector<float>& skinned_points,
    std::vector<float>& skinned_normals)
{
    const auto& points = skinned_mesh.GetSkinningPoints();
    const auto& normals = skinned_mesh.GetSkinningNormals();
    const auto& bone_indices = skinned_mesh.GetSkinningBoneIndices();
    const auto& bone_weights = skinned_mesh.GetSkinningBoneWeights();
    const std::size_t vertex_count = points.size() / 3u;
    if (vertex_count == 0 || bone_indices.size() < vertex_count * 4u ||
        bone_weights.size() < vertex_count * 4u)
    {
        return false;
    }

    const auto bone_matrices =
        skinned_mesh.EvaluateBoneMatrices(skinning_time_s);
    if (bone_matrices.empty())
    {
        return false;
    }

    skinned_points.assign(points.size(), 0.0f);
    skinned_normals.assign(normals.size(), 0.0f);
    for (std::size_t vertex = 0; vertex < vertex_count; ++vertex)
    {
        const std::size_t point_offset = vertex * 3u;
        const std::size_t bone_offset = vertex * 4u;
        glm::mat4 skin_matrix(0.0f);
        float weight_sum = 0.0f;
        for (std::size_t slot = 0; slot < 4u; ++slot)
        {
            const float weight = bone_weights[bone_offset + slot];
            if (weight <= 0.0f)
            {
                continue;
            }
            const auto bone_index = bone_indices[bone_offset + slot];
            if (bone_index < 0 ||
                bone_index >= static_cast<std::int32_t>(bone_matrices.size()))
            {
                continue;
            }
            skin_matrix +=
                bone_matrices[static_cast<std::size_t>(bone_index)] * weight;
            weight_sum += weight;
        }
        if (weight_sum <= 0.0f)
        {
            skin_matrix = glm::mat4(1.0f);
        }

        const glm::vec4 point = skin_matrix * glm::vec4(
                                                  points[point_offset + 0u],
                                                  points[point_offset + 1u],
                                                  points[point_offset + 2u],
                                                  1.0f);
        skinned_points[point_offset + 0u] = point.x;
        skinned_points[point_offset + 1u] = point.y;
        skinned_points[point_offset + 2u] = point.z;

        if (point_offset + 2u < normals.size())
        {
            glm::vec3 normal =
                glm::mat3(skin_matrix) * glm::vec3(
                                             normals[point_offset + 0u],
                                             normals[point_offset + 1u],
                                             normals[point_offset + 2u]);
            if (glm::length(normal) > 1.0e-6f)
            {
                normal = glm::normalize(normal);
            }
            skinned_normals[point_offset + 0u] = normal.x;
            skinned_normals[point_offset + 1u] = normal.y;
            skinned_normals[point_offset + 2u] = normal.z;
        }
    }
    return true;
}

} // namespace

MeshResources::MeshResources(
    vk::Device device,
    GpuMemoryManager& memory_manager,
    CommandQueue& command_queue,
    const Logger& logger)
    : device_(device), memory_manager_(&memory_manager),
      command_queue_(&command_queue), logger_(&logger)
{
}

void MeshResources::Build(
    const frame::LevelInterface& level,
    const frame::json::LevelData& level_data,
    bool prefer_level_scene_meshes)
{
    std::vector<MeshBuildInfo> mesh_infos = {};
    if (prefer_level_scene_meshes)
    {
        mesh_infos =
            ExtractMeshes(level, frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
        auto scene_meshes =
            ExtractMeshes(level, frame::proto::NodeMesh::SCENE_RENDER_TIME);
        mesh_infos.insert(
            mesh_infos.end(),
            std::make_move_iterator(scene_meshes.begin()),
            std::make_move_iterator(scene_meshes.end()));
    }
    UploadMeshes(std::move(mesh_infos), level_data);
}

void MeshResources::Build(const frame::json::LevelData& level_data)
{
    UploadMeshes({}, level_data);
}

void MeshResources::UploadMeshes(
    std::vector<MeshBuildInfo> mesh_infos,
    const frame::json::LevelData& level_data)
{
    meshes_.clear();

    if (mesh_infos.empty())
    {
        mesh_infos.reserve(level_data.meshes.size());
        for (const auto& mesh_info : level_data.meshes)
        {
            mesh_infos.push_back(
                {.mesh_info = mesh_info, .source_node_id = NullId});
        }
    }
    if (mesh_infos.empty())
    {
        mesh_infos.push_back(MakeFallbackQuad());
    }

    for (const auto& build_info : mesh_infos)
    {
        try
        {
            auto resource = BuildMeshResource(build_info.mesh_info);
            resource.source_node_id = build_info.source_node_id;
            resource.source_mesh_id = build_info.source_mesh_id;
            resource.source_material_id = build_info.source_material_id;
            resource.render_time = build_info.render_time;
            meshes_.push_back(std::move(resource));
        }
        catch (const std::exception& ex)
        {
            (*logger_)->warn(
                "Skipping mesh {}: {}", build_info.mesh_info.name, ex.what());
        }
    }
}

void MeshResources::UpdateAnimatedMeshes(
    const frame::LevelInterface& level, double time_s)
{
    for (auto& resource : meshes_)
    {
        if (resource.source_mesh_id == frame::NullId)
        {
            continue;
        }

        const auto* skinned_mesh =
            dynamic_cast<const frame::vulkan::SkinnedMesh*>(
                &level.GetMeshFromId(resource.source_mesh_id));
        if (!skinned_mesh || !skinned_mesh->IsSkinningAnimationEnabled() ||
            !skinned_mesh->HasGpuSkinningSourceData() ||
            !skinned_mesh->HasBoneMatricesCallback())
        {
            continue;
        }

        const double skinning_time_s = skinned_mesh->GetSkinningTime(time_s);
        if (resource.last_skinning_time_s >= 0.0 &&
            std::abs(resource.last_skinning_time_s - skinning_time_s) < 1.0e-5)
        {
            continue;
        }

        frame::json::StaticMeshInfo mesh_info = {};
        mesh_info.name = level.GetNameFromId(resource.source_mesh_id);
        mesh_info.uvs = skinned_mesh->GetSkinningTextures();
        mesh_info.indices = skinned_mesh->GetSkinningTriangleIndices();
        if (!EvaluateSkinnedRasterMesh(
                *skinned_mesh,
                skinning_time_s,
                mesh_info.positions,
                mesh_info.normals))
        {
            continue;
        }

        const auto vertices = BuildMeshVertices(mesh_info);
        const vk::DeviceSize vertex_size =
            static_cast<vk::DeviceSize>(vertices.size() * sizeof(MeshVertex));
        if (vertex_size != resource.vertex_size_bytes)
        {
            (*logger_)->warn(
                "Skipping animated mesh update for {}: vertex size changed.",
                mesh_info.name);
            continue;
        }
        UploadVertexData(resource, vertices);
        resource.last_skinning_time_s = skinning_time_s;
    }
}

void MeshResources::Clear()
{
    meshes_.clear();
}

std::vector<MeshResources::MeshBuildInfo> MeshResources::ExtractMeshes(
    const frame::LevelInterface& level,
    frame::proto::NodeMesh::RenderTimeEnum render_time)
{
    std::vector<MeshBuildInfo> mesh_infos = {};
    for (const auto& [node_id, material_id] :
         level.GetMeshMaterialIds(render_time))
    {
        if (node_id == frame::NullId)
        {
            continue;
        }
        const auto& node = level.GetSceneNodeFromId(node_id);
        const auto mesh_id = node.GetLocalMesh();
        if (mesh_id == frame::NullId)
        {
            continue;
        }
        const auto& mesh = level.GetMeshFromId(mesh_id);
        if (mesh.GetData().render_primitive_enum() !=
            frame::proto::NodeMesh::TRIANGLE_PRIMITIVE)
        {
            continue;
        }
        if (mesh.GetPointBufferId() == frame::NullId ||
            mesh.GetIndexBufferId() == frame::NullId)
        {
            continue;
        }

        frame::json::StaticMeshInfo mesh_info = {};
        mesh_info.name = level.GetNameFromId(mesh_id);
        mesh_info.positions = ReadVectorFromBuffer<float>(
            level.GetBufferFromId(mesh.GetPointBufferId()));
        if (mesh.GetNormalBufferId() != frame::NullId)
        {
            mesh_info.normals = ReadVectorFromBuffer<float>(
                level.GetBufferFromId(mesh.GetNormalBufferId()));
        }
        if (mesh.GetTextureBufferId() != frame::NullId)
        {
            mesh_info.uvs = ReadVectorFromBuffer<float>(
                level.GetBufferFromId(mesh.GetTextureBufferId()));
        }
        mesh_info.indices = ReadVectorFromBuffer<std::uint32_t>(
            level.GetBufferFromId(mesh.GetIndexBufferId()));
        if (!mesh_info.positions.empty() && !mesh_info.indices.empty())
        {
            mesh_infos.push_back(
                {.mesh_info = std::move(mesh_info),
                 .source_node_id = node_id,
                 .source_mesh_id = mesh_id,
                 .source_material_id = material_id,
                 .render_time = render_time});
        }
    }
    return mesh_infos;
}

MeshResources::MeshBuildInfo MeshResources::MakeFallbackQuad()
{
    frame::json::StaticMeshInfo quad{};
    quad.positions = {
        -1.0f,
        -1.0f,
        0.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        -1.0f,
        1.0f,
        0.0f};
    quad.uvs = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    quad.normals = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    quad.indices = {0, 1, 2, 2, 3, 0};
    quad.name = "FallbackQuad";
    return {.mesh_info = std::move(quad), .source_node_id = NullId};
}

MeshResource MeshResources::BuildMeshResource(
    const frame::json::StaticMeshInfo& mesh_info)
{
    auto vertices = BuildMeshVertices(mesh_info);
    if (vertices.empty())
    {
        throw std::runtime_error("Static mesh has no vertices.");
    }

    MeshResource resource;
    const vk::DeviceSize vertex_size =
        static_cast<vk::DeviceSize>(vertices.size() * sizeof(MeshVertex));
    resource.vertex_size_bytes = vertex_size;

    vk::UniqueDeviceMemory staging_vertex_memory;
    auto staging_vertex_buffer = memory_manager_->CreateBuffer(
        vertex_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_vertex_memory);

    void* mapped_vertices =
        device_.mapMemory(*staging_vertex_memory, 0, vertex_size);
    std::memcpy(mapped_vertices, vertices.data(), vertex_size);
    device_.unmapMemory(*staging_vertex_memory);

    auto vertex_buffer = memory_manager_->CreateBuffer(
        vertex_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        resource.vertex_memory);
    command_queue_->CopyBuffer(
        *staging_vertex_buffer, *vertex_buffer, vertex_size);
    resource.vertex_buffer = std::move(vertex_buffer);

    const auto& indices = mesh_info.indices;
    if (!indices.empty())
    {
        const vk::DeviceSize index_size =
            static_cast<vk::DeviceSize>(indices.size() * sizeof(std::uint32_t));
        vk::UniqueDeviceMemory staging_index_memory;
        auto staging_index_buffer = memory_manager_->CreateBuffer(
            index_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_index_memory);

        void* mapped_indices =
            device_.mapMemory(*staging_index_memory, 0, index_size);
        std::memcpy(mapped_indices, indices.data(), index_size);
        device_.unmapMemory(*staging_index_memory);

        auto index_buffer = memory_manager_->CreateBuffer(
            index_size,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            resource.index_memory);
        command_queue_->CopyBuffer(
            *staging_index_buffer, *index_buffer, index_size);
        resource.index_buffer = std::move(index_buffer);
        resource.index_count = static_cast<std::uint32_t>(indices.size());
    }
    else
    {
        resource.index_count = static_cast<std::uint32_t>(vertices.size());
    }

    return resource;
}

void MeshResources::UploadVertexData(
    MeshResource& resource, const std::vector<MeshVertex>& vertices)
{
    if (vertices.empty() || !resource.vertex_buffer)
    {
        return;
    }

    const vk::DeviceSize vertex_size =
        static_cast<vk::DeviceSize>(vertices.size() * sizeof(MeshVertex));
    if (vertex_size != resource.vertex_size_bytes)
    {
        return;
    }

    vk::UniqueDeviceMemory staging_vertex_memory;
    auto staging_vertex_buffer = memory_manager_->CreateBuffer(
        vertex_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_vertex_memory);

    void* mapped_vertices =
        device_.mapMemory(*staging_vertex_memory, 0, vertex_size);
    std::memcpy(mapped_vertices, vertices.data(), vertex_size);
    device_.unmapMemory(*staging_vertex_memory);

    command_queue_->CopyBuffer(
        *staging_vertex_buffer, *resource.vertex_buffer, vertex_size);
}

} // namespace frame::vulkan
