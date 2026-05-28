#pragma once

#include "frame/backend_internal.h"

#include <vector>

#include "frame/vulkan/vulkan_dispatch.h"

#include "frame/json/level_data.h"
#include "frame/level_interface.h"
#include "frame/logger.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_utils.h"

namespace frame::vulkan
{

struct MeshResource
{
    vk::UniqueBuffer vertex_buffer;
    vk::UniqueDeviceMemory vertex_memory;
    vk::UniqueBuffer index_buffer;
    vk::UniqueDeviceMemory index_memory;
    EntityId source_node_id = NullId;
    EntityId source_mesh_id = NullId;
    EntityId source_material_id = NullId;
    std::uint32_t index_count = 0;
    vk::DeviceSize vertex_size_bytes = 0;
    double last_skinning_time_s = -1.0;
    frame::proto::NodeMesh::RenderTimeEnum render_time =
        frame::proto::NodeMesh::SCENE_RENDER_TIME;
};

class MeshResources
{
  public:
    MeshResources(
        vk::Device device,
        GpuMemoryManager& memory_manager,
        CommandQueue& command_queue,
        const Logger& logger);

    void Build(
        const frame::LevelInterface& level,
        const frame::json::LevelData& level_data,
        bool prefer_level_scene_meshes);
    void Build(const frame::json::LevelData& level_data);
    void UpdateAnimatedMeshes(
        const frame::LevelInterface& level, double time_s);
    void Clear();

    const std::vector<MeshResource>& GetMeshes() const
    {
        return meshes_;
    }

    bool Empty() const
    {
        return meshes_.empty();
    }

  private:
    struct MeshBuildInfo
    {
        frame::json::StaticMeshInfo mesh_info;
        EntityId source_node_id = NullId;
        EntityId source_mesh_id = NullId;
        EntityId source_material_id = NullId;
        frame::proto::NodeMesh::RenderTimeEnum render_time =
            frame::proto::NodeMesh::SCENE_RENDER_TIME;
    };

    MeshResource BuildMeshResource(
        const frame::json::StaticMeshInfo& mesh_info);
    void UploadVertexData(
        MeshResource& resource, const std::vector<MeshVertex>& vertices);
    void UploadMeshes(
        std::vector<MeshBuildInfo> mesh_infos,
        const frame::json::LevelData& level_data);
    static std::vector<MeshBuildInfo> ExtractMeshes(
        const frame::LevelInterface& level,
        frame::proto::NodeMesh::RenderTimeEnum render_time);
    static MeshBuildInfo MakeFallbackQuad();

    vk::Device device_;
    GpuMemoryManager* memory_manager_;
    CommandQueue* command_queue_;
    const Logger* logger_;
    std::vector<MeshResource> meshes_;
};

} // namespace frame::vulkan
