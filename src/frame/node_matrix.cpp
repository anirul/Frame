#include "frame/node_matrix.h"

#include <stdexcept>

namespace frame {

glm::mat4 NodeMatrix::GetLocalModel(const double dt) const {
    if (!GetParentName().empty()) {
        if (!func_("root")) {
            throw std::runtime_error("Should initiate NodeInterface correctly!");
        }
        auto parent_node = func_(GetParentName());
        if (!parent_node) {
            throw std::runtime_error("SceneMatrix func(" + GetParentName() + ") returned nullptr");
        }
        return parent_node->GetLocalModel(dt) * ComputeLocalRotation(dt);
    }
    return ComputeLocalRotation(dt);
}

// FIXME(anirul): Find a better way.
glm::mat4 NodeMatrix::ComputeLocalRotation(const double dt) const {
    if (matrix_ == glm::mat4(1.0f) || !enable_rotation_) return matrix_;
    glm::quat rotation = glm::quat_cast(matrix_);
    // should decode the angle and recode it.
    float normal =
        std::sqrt(rotation.x * rotation.x + rotation.y * rotation.y + rotation.z * rotation.z);
    float angle_rad = 2.0f * std::atan2(normal, rotation.w);
    glm::vec3 axis  = glm::vec3(rotation.x / normal, rotation.y / normal, rotation.z / normal);
    // Reconvert from angle / axis to quaternion.
    glm::quat present_rotation =
        glm::angleAxis(angle_rad * static_cast<float>(dt), glm::normalize(axis));
    glm::mat4 result_matrix = glm::toMat4(present_rotation);
    return result_matrix;
}

}  // End namespace frame.
