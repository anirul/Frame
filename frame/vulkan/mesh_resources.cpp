#include "frame/vulkan/mesh_resources.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace frame::vulkan
{

MeshResources::MeshResources(
    vk::Device device,
    GpuMemoryManager& memory_manager,
    CommandQueue& command_queue,
    const Logger& logger)
    : device_(device),
      memory_manager_(&memory_manager),
      command_queue_(&command_queue),
      logger_(&logger)
{
}

void MeshResources::Build(const frame::json::LevelData& level_data)
{
    meshes_.clear();

    std::vector<frame::json::StaticMeshInfo> mesh_infos = level_data.meshes;
    if (mesh_infos.empty())
    {
        mesh_infos.push_back(MakeFallbackQuad());
    }

    for (const auto& mesh_info : mesh_infos)
    {
        try
        {
            meshes_.push_back(BuildMeshResource(mesh_info));
        }
        catch (const std::exception& ex)
        {
            (*logger_)->warn(
                "Skipping mesh {}: {}",
                mesh_info.name,
                ex.what());
        }
    }
}

void MeshResources::Clear()
{
    meshes_.clear();
}

frame::json::StaticMeshInfo MeshResources::MakeFallbackQuad()
{
    frame::json::StaticMeshInfo quad{};
    quad.positions = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f};
    quad.uvs = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
    quad.indices = {0, 1, 2, 2, 3, 0};
    quad.name = "FallbackQuad";
    return quad;
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
            static_cast<vk::DeviceSize>(
                indices.size() * sizeof(std::uint32_t));
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

} // namespace frame::vulkan
