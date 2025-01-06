#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "frame/light_interface.h"
#include "frame/node_interface.h"

namespace frame
{

/**
 * @class NodeLight
 * @brief A light in the node for the scene tree.
 */
class NodeLight : public NodeInterface
{
  public:
    /**
     * @brief Create an ambient light.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
     * @param color: Color of the light in vec3 format.
	 * @remark No shadow as this is the ambient light.
     */
    NodeLight(
        std::function<NodeInterface*(const std::string&)> func,
        const glm::vec3 color)
        : NodeInterface(func),
		  light_type_(LightTypeEnum::AMBIENT_LIGHT),
          color_(color)
    {
    }
    /**
     * @brief Create a point or directional light.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
     * @param light_type: Light type of the light.
     * @param position_or_direction: Position (if point light) or direction
     *        (if directional light).
     * @param color: Color of the light in vec3 format.
     */
    NodeLight(
        std::function<NodeInterface*(const std::string&)> func,
        const frame::LightTypeEnum light_type,
        const glm::vec3 position_or_direction,
        const glm::vec3 color);
    /**
     * @brief Create a point or directional light with shadow.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
     * @param light_type: Light type of the light.
	 * @param shadow_type: Type of shadow used.
	 * @param shadow_texture: Name of the texture to render for the shadows.
     * @param position_or_direction: Position (if point light) or direction
     *        (if directional light).
     * @param color: Color of the light in vec3 format.
     */
    NodeLight(
		std::function<NodeInterface*(const std::string&)> func,
		const frame::LightTypeEnum light_type,
		const frame::ShadowTypeEnum shadow_type,
        const std::string& shadow_texture,
		const glm::vec3 position_or_direction,
		const glm::vec3 color);
    /**
     * @brief Create a spot light.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
     * @param position: Position of the spot light.
     * @param direction: Direction of the spot light.
     * @param color: Color in a vec3 format.
     * @param dot_inner_limit: Inner limit of the total light in dot format.
     * @param dot_outer_limit: Outer limit of the total light in dot format.
     */
    NodeLight(
        std::function<NodeInterface*(const std::string&)> func,
        const glm::vec3 position,
        const glm::vec3 direction,
        const glm::vec3 color,
        const float dot_inner_limit,
        const float dot_outer_limit);
    /**
     * @brief Create a spot light.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
 	 * @param shadow_type: Type of shadow used.
     * @param shadow_texture: Name of the texture to render for the shadows.
     * @param position: Position of the spot light.
     * @param direction: Direction of the spot light.
     * @param color: Color in a vec3 format.
     * @param dot_inner_limit: Inner limit of the total light in dot format.
     * @param dot_outer_limit: Outer limit of the total light in dot format.
     */
    NodeLight(
        std::function<NodeInterface*(const std::string&)> func,
		ShadowTypeEnum shadow_type,
		const std::string& shadow_texture,
        const glm::vec3 position,
        const glm::vec3 direction,
        const glm::vec3 color,
        const float dot_inner_limit,
        const float dot_outer_limit);
    //! @brief Virtual destructor.
    ~NodeLight() override = default;

  public:
    /**
     * @brief Compute the local model of current node.
     * @param dt: Delta time from the beginning of the software running in
     * seconds.
     * @return A mat4 representing the local model matrix.
     */
    glm::mat4 GetLocalModel(const double dt) const override;

  public:
    /**
     * @brief Get the light type.
     * @return The light type (see the NodeLightEnum).
     */
    LightTypeEnum GetType() const
    {
        return light_type_;
    }
    /**
     * @brief Get the light position.
     * @return the light position.
     */
    glm::vec3 GetPosition() const
    {
        return position_;
    }
    /**
     * @brief Get the light direction.
     * @return the light direction.
     */
    glm::vec3 GetDirection() const
    {
        return direction_;
    }
    /**
     * @brief Get the light color.
     * @return the light color.
     */
    glm::vec3 GetColor() const
    {
        return color_;
    }
    /**
     * @brief Get the inner limit in dot format.
     * @return the inner limit in dot format.
     */
    float GetDotInner() const
    {
        return dot_inner_limit_;
    }
    /**
     * @brief Get the outer limit in dot format.
     * @return the outer limit in dot format.
     */
    float GetDotOuter() const
    {
        return dot_outer_limit_;
    }

  private:
    LightTypeEnum light_type_ = LightTypeEnum::INVALID_LIGHT;
	ShadowTypeEnum shadow_type_ = ShadowTypeEnum::NO_SHADOW;
    std::string shadow_texture_ = "";
    glm::vec3 position_ = glm::vec3(0.0f);
    glm::vec3 direction_ = glm::vec3(0.0f);
    glm::vec3 color_ = glm::vec3(1.0f);
    float dot_inner_limit_ = 0.0f;
    float dot_outer_limit_ = 0.0f;
};

} // End namespace frame.
