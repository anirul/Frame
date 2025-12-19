#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include <vector>
#include <vulkan/vulkan.hpp>

#include "frame/json/level_data.h"
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
    std::uint32_t index_count = 0;
};

class MeshResources
{
  public:
    MeshResources(
        vk::Device device,
        GpuMemoryManager& memory_manager,
        CommandQueue& command_queue,
        const Logger& logger);

    void Build(const frame::json::LevelData& level_data);
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
    MeshResource BuildMeshResource(
        const frame::json::StaticMeshInfo& mesh_info);
    static frame::json::StaticMeshInfo MakeFallbackQuad();

    vk::Device device_;
    GpuMemoryManager* memory_manager_;
    CommandQueue* command_queue_;
    const Logger* logger_;
    std::vector<MeshResource> meshes_;
};

} // namespace frame::vulkan
