#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "frame/json/level_data.h"

namespace frame::vulkan
{

struct MeshVertex
{
    glm::vec3 position{};
    glm::vec2 uv{};
};

std::vector<MeshVertex> BuildMeshVertices(
    const frame::json::StaticMeshInfo& mesh_info);

}
