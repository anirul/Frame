#include "frame/node_mesh.h"

#include <format>

#include <stdexcept>

namespace frame
{

NodeMesh::~NodeMesh() = default;

glm::mat4 NodeMesh::GetLocalModel(const double dt) const
{
    if (!GetParentName().empty())
    {
        auto parent_node = func_(GetParentName());
        if (!parent_node)
        {
            throw std::runtime_error(
                std::format(
                    "SceneMesh func({}) returned nullptr",
                    GetParentName()));
        }
        return parent_node->GetLocalModel(dt);
    }
    return glm::mat4(1.0f);
}

} // End namespace frame.

