#include "frame/node_camera.h"

#include <format>
#include <stdexcept>

namespace frame
{

NodeCamera::~NodeCamera() = default;

glm::mat4 NodeCamera::GetLocalModel(const double dt) const
{
    if (!GetParentName().empty())
    {
        auto parent_node = func_(GetParentName());
        if (!parent_node)
        {
            throw std::runtime_error(
                std::format(
                    "SceneCamera func({}) returned nullptr", GetParentName()));
        }
        return parent_node->GetLocalModel(dt);
    }
    return glm::mat4(1.0f);
}

} // End namespace frame.
