#include "frame/node_light.h"

#include <fmt/core.h>
#include <stdexcept>

namespace frame
{

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
    const glm::vec3 color) :
    NodeInterface(func),
    light_type_(light_type),
    color_(color)
{
    if (light_type_ == LightTypeEnum::POINT_LIGHT)
    {
        position_ = position_or_direction;
    }
    else if (light_type_ == LightTypeEnum::DIRECTIONAL_LIGHT)
    {
        direction_ = position_or_direction;
    }
    else
    {
        std::string value = std::to_string(static_cast<int>(light_type));
        throw std::runtime_error("illegal light(" + value + ")");
    }
}

NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const frame::LightTypeEnum light_type,
    const frame::ShadowTypeEnum shadow_type,
    const std::string& shadow_texture,
    const glm::vec3 position_or_direction,
    const glm::vec3 color)
    : NodeInterface(func),
      light_type_(light_type),
      color_(color)
{
    if (light_type_ == LightTypeEnum::POINT_LIGHT)
    {
        position_ = position_or_direction;
    }
    else if (light_type_ == LightTypeEnum::DIRECTIONAL_LIGHT)
    {
        direction_ = position_or_direction;
    }
    else
    {
        std::string value = std::to_string(static_cast<int>(light_type));
        throw std::runtime_error("illegal light(" + value + ")");
    }
    if (shadow_type != ShadowTypeEnum::NO_SHADOW)
    {
        shadow_type_ = shadow_type;
        shadow_texture_ = shadow_texture;
    }
}


NodeLight::NodeLight(
    std::function<NodeInterface*(const std::string&)> func,
    const glm::vec3 position,
    const glm::vec3 direction,
    const glm::vec3 color,
    const float dot_inner_limit,
    const float dot_outer_limit)
    : NodeInterface(func),
      light_type_(LightTypeEnum::SPOT_LIGHT),
      position_(position),
      direction_(direction),
      color_(color),
      dot_inner_limit_(dot_inner_limit),
      dot_outer_limit_(dot_outer_limit)
{
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
    : NodeInterface(func),
      light_type_(LightTypeEnum::SPOT_LIGHT),
      shadow_type_(shadow_type),
      shadow_texture_(shadow_texture),
      position_(position),
      direction_(direction),
      color_(color),
      dot_inner_limit_(dot_inner_limit),
      dot_outer_limit_(dot_outer_limit)
{
}

glm::mat4 NodeLight::GetLocalModel(const double dt) const
{
    if (!GetParentName().empty())
    {
        auto parent_node = func_(GetParentName());
        if (!parent_node)
        {
            throw std::runtime_error(fmt::format(
                "SceneLight func({}) returned nullptr", GetParentName()));
        }
        return parent_node->GetLocalModel(dt);
    }
    return glm::mat4(1.0f);
}

} // End namespace frame.
