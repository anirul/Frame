#pragma once

#include <glm/glm.hpp>
#include "Frame/NodeInterface.h"

namespace frame {

	enum class NodeLightEnum : std::uint8_t {
		INVALID = 0,
		AMBIENT = 1,
		POINT = 2,
		DIRECTIONAL = 3,
		SPOT = 4
	};

	class NodeLight : public NodeInterface
	{
	public:
		// Create an ambient light.
		NodeLight(const glm::vec3 color) :
			light_type_(NodeLightEnum::AMBIENT), color_(color) {}
		// Create a point or directional light.
		NodeLight(
			const frame::NodeLightEnum light_type,
			const glm::vec3 position_or_direction,
			const glm::vec3 color);
		// Create a spot light.
		NodeLight(
			const glm::vec3 position,
			const glm::vec3 direction,
			const glm::vec3 color,
			const float dot_inner_limit,
			const float dot_outer_limit);

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;

	public:
		const NodeLightEnum GetType() const
		{
			return light_type_;
		}
		const glm::vec3 GetPosition() const { return position_; }
		const glm::vec3 GetDirection() const { return direction_; }
		const glm::vec3 GetColor() const { return color_; }
		const float GetDotInner() const { return dot_inner_limit_; }
		const float GetDotOuter() const { return dot_outer_limit_; }

	private:
		NodeLightEnum light_type_ = NodeLightEnum::INVALID;
		glm::vec3 position_ = glm::vec3(0.0f);
		glm::vec3 direction_ = glm::vec3(0.0f);
		glm::vec3 color_ = glm::vec3(1.0f);
		float dot_inner_limit_ = 0.0f;
		float dot_outer_limit_ = 0.0f;
	};

} // End namespace frame.
