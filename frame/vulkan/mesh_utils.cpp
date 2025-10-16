#include "frame/vulkan/mesh_utils.h"

#include <stdexcept>

namespace frame::vulkan
{

std::vector<MeshVertex> BuildMeshVertices(
    const frame::json::StaticMeshInfo& mesh_info)
{
    const auto& positions = mesh_info.positions;
    const auto& uvs = mesh_info.uvs;
    if (positions.empty() || positions.size() % 3 != 0)
    {
        throw std::runtime_error("Static mesh positions must be non-empty and a multiple of 3.");
    }

    const std::size_t vertex_count = positions.size() / 3;
    std::vector<MeshVertex> vertices(vertex_count);
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        MeshVertex vertex{};
        vertex.position = glm::vec3(
            positions[i * 3 + 0],
            positions[i * 3 + 1],
            positions[i * 3 + 2]);
        if (uvs.size() >= (i + 1) * 2)
        {
            vertex.uv = glm::vec2(
                uvs[i * 2 + 0],
                uvs[i * 2 + 1]);
        }
        else
        {
            vertex.uv = glm::vec2(0.0f, 0.0f);
        }
        vertices[i] = vertex;
    }
    return vertices;
}

} // namespace frame::vulkan
