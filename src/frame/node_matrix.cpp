#include "frame/node_matrix.h"

#include "frame/json/parse_uniform.h"
#include "frame/json/serialize_uniform.h"
#include <fmt/core.h>
#include <stdexcept>

namespace frame
{

NodeMatrix::~NodeMatrix() = default;

NodeMatrix::NodeMatrix(
    std::function<NodeInterface*(const std::string&)> func,
    glm::mat4 matrix,
    bool rotation)
    : NodeInterface(func)
{
    data_.mutable_matrix()->CopyFrom(json::SerializeUniformMatrix4(matrix));
    if (rotation)
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::ROTATION_MATRIX);
    }
    else
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::STATIC_MATRIX);
    }
}

NodeMatrix::NodeMatrix(
    std::function<NodeInterface*(const std::string&)> func,
    glm::vec4 quat,
    bool rotation)
    : NodeInterface(func)
{
    data_.mutable_quaternion()->CopyFrom(json::SerializeUniformVector4(quat));
    if (rotation)
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::ROTATION_MATRIX);
    }
    else
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::STATIC_MATRIX);
    }
}

NodeMatrix::NodeMatrix(glm::mat4 matrix, bool rotation)
    : NodeInterface([](std::string) { return nullptr; })
{
    data_.mutable_matrix()->CopyFrom(json::SerializeUniformMatrix4(matrix));
    if (rotation)
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::ROTATION_MATRIX);
    }
    else
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::STATIC_MATRIX);
    }
}

NodeMatrix::NodeMatrix(glm::vec4 quat, bool rotation)
    : NodeInterface([](std::string) { return nullptr; })
{
    data_.mutable_quaternion()->CopyFrom(json::SerializeUniformVector4(quat));
    if (rotation)
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::ROTATION_MATRIX);
    }
    else
    {
        data_.set_matrix_type_enum(proto::NodeMatrix::STATIC_MATRIX);
    }
}

glm::mat4 NodeMatrix::GetLocalModel(const double dt) const
{
    if (!GetParentName().empty())
    {
        if (!func_("root"))
        {
            throw std::runtime_error(
                "Should initiate NodeInterface correctly!");
        }
        auto parent_node = func_(GetParentName());
        if (!parent_node)
        {
            throw std::runtime_error(
                fmt::format(
                    "SceneMatrix func({}) returned nullptr", GetParentName()));
        }
        return parent_node->GetLocalModel(dt) * ComputeLocalRotation(dt);
    }
    return ComputeLocalRotation(dt);
}

// FIXME(anirul): Find a better way, this doesn't work for quaternion?
glm::mat4 NodeMatrix::ComputeLocalRotation(const double dt) const
{
    glm::mat4 glm_matrix = json::ParseUniform(data_.matrix());
    if (glm_matrix == glm::mat4(1.0) ||
        GetData().matrix_type_enum() == proto::NodeMatrix::STATIC_MATRIX)
    {
        return glm_matrix;
    }
    glm::quat rotation = glm::quat_cast(glm_matrix);
    // should decode the angle and recode it.
    float normal = std::sqrt(
        rotation.x * rotation.x + rotation.y * rotation.y +
        rotation.z * rotation.z);
    float angle_rad = 2.0f * std::atan2(normal, rotation.w);
    glm::vec3 axis = glm::vec3(
        rotation.x / normal, rotation.y / normal, rotation.z / normal);
    // Reconvert from angle / axis to quaternion.
    glm::quat present_rotation = glm::angleAxis(
        angle_rad * static_cast<float>(dt), glm::normalize(axis));
    glm::mat4 result_matrix = glm::toMat4(present_rotation);
    return result_matrix;
}

} // End namespace frame.
