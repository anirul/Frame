#include "frame/node_light.h"

#include "frame/json/serialize_uniform.h"

#include <fmt/core.h>
#include <stdexcept>

namespace frame
{

NodeLight::~NodeLight() = default;

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const glm::vec3 color)
    : NodeInterface(func)
{
    data_.set_light_type(proto::NodeLight::AMBIENT_LIGHT);
    proto::UniformVector3 proto_uniform_vector3;
    proto_uniform_vector3.set_x(color.x);
    proto_uniform_vector3.set_y(color.y);
    proto_uniform_vector3.set_z(color.z);
    data_.mutable_color()->CopyFrom(proto_uniform_vector3);
}

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const LightTypeEnum light_type,
    const glm::vec3 position_or_direction,
    const glm::vec3 color)
    : NodeInterface(func)
{
    data_.set_light_type(
        static_cast<proto::NodeLight::LightTypeEnum>(light_type));
    data_.mutable_color()->CopyFrom(json::SerializeUniformVector3(color));
    switch (data_.light_type())
    {
    case proto::NodeLight::POINT_LIGHT:
        data_.mutable_position()->CopyFrom(
            json::SerializeUniformVector3(position_or_direction));
        break;
    case proto::NodeLight::DIRECTIONAL_LIGHT:
        data_.mutable_direction()->CopyFrom(
            json::SerializeUniformVector3(position_or_direction));
        break;
    default: {
        std::string value = std::to_string(static_cast<int>(light_type));
        throw std::runtime_error("illegal light(" + value + ")");
    }
    }
}

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const frame::LightTypeEnum light_type,
    const frame::ShadowTypeEnum shadow_type,
    const std::string& shadow_texture,
    const glm::vec3 position_or_direction,
    const glm::vec3 color)
    : NodeInterface(func)
{
    data_.set_light_type(
        static_cast<proto::NodeLight::LightTypeEnum>(light_type));
    data_.mutable_color()->CopyFrom(json::SerializeUniformVector3(color));
    switch (data_.light_type())
    {
    case proto::NodeLight::POINT_LIGHT:
        data_.mutable_position()->CopyFrom(
            json::SerializeUniformVector3(position_or_direction));
        break;
    case proto::NodeLight::DIRECTIONAL_LIGHT:
        data_.mutable_direction()->CopyFrom(
            json::SerializeUniformVector3(position_or_direction));
        break;
    default: {
        std::string value = std::to_string(static_cast<int>(light_type));
        throw std::runtime_error("illegal light(" + value + ")");
    }
    }
    if (shadow_type != ShadowTypeEnum::NO_SHADOW)
    {
        data_.set_shadow_type(
            static_cast<proto::NodeLight::ShadowTypeEnum>(shadow_type));
        data_.set_shadow_texture(shadow_texture);
    }
}

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const glm::vec3 position,
    const glm::vec3 direction,
    const glm::vec3 color,
    const float dot_inner_limit,
    const float dot_outer_limit)
    : NodeInterface(func)
{
    data_.set_light_type(
        static_cast<proto::NodeLight::LightTypeEnum>(
            LightTypeEnum::SPOT_LIGHT));
    data_.mutable_position()->CopyFrom(json::SerializeUniformVector3(position));
    data_.mutable_direction()->CopyFrom(
        json::SerializeUniformVector3(direction));
    data_.mutable_color()->CopyFrom(json::SerializeUniformVector3(color));
    data_.set_dot_inner_limit(dot_inner_limit);
    data_.set_dot_outer_limit(dot_outer_limit);
}

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    ShadowTypeEnum shadow_type,
    const std::string& shadow_texture,
    const glm::vec3 position,
    const glm::vec3 direction,
    const glm::vec3 color,
    const float dot_inner_limit,
    const float dot_outer_limit)
    : NodeInterface(func)
{
    data_.set_light_type(
        static_cast<proto::NodeLight::LightTypeEnum>(
            LightTypeEnum::SPOT_LIGHT));
    data_.mutable_position()->CopyFrom(json::SerializeUniformVector3(position));
    data_.mutable_direction()->CopyFrom(
        json::SerializeUniformVector3(direction));
    data_.mutable_color()->CopyFrom(json::SerializeUniformVector3(color));
    data_.set_dot_inner_limit(dot_inner_limit);
    data_.set_dot_outer_limit(dot_outer_limit);
    if (shadow_type != ShadowTypeEnum::NO_SHADOW)
    {
        data_.set_shadow_type(
            static_cast<proto::NodeLight::ShadowTypeEnum>(shadow_type));
        data_.set_shadow_texture(shadow_texture);
    }
}

glm::mat4 NodeLight::GetLocalModel(const double dt) const
{
    if (!GetParentName().empty())
    {
        auto parent_node = func_(GetParentName());
        if (!parent_node)
        {
            throw std::runtime_error(
                fmt::format(
                    "SceneLight func({}) returned nullptr", GetParentName()));
        }
        return parent_node->GetLocalModel(dt);
    }
    return glm::mat4(1.0f);
}

} // End namespace frame.
