#include "SceneLight.h"
#include <stdexcept>

namespace frame {
	
	SceneLight::SceneLight(
		const SceneLightEnum light_type,
		const glm::vec3 position_or_direction,
		const glm::vec3 color) :
		light_type_(light_type),
		color_(color)
	{
		if (light_type_ == SceneLightEnum::POINT)
		{
			position_ = position_or_direction;
		}
		else if (light_type_ == SceneLightEnum::DIRECTIONAL)
		{
			direction_ = position_or_direction;
		}
		else
		{
			std::string value = std::to_string(static_cast<int>(light_type));
			throw std::runtime_error("illegal light(" + value + ")");
		}
	}

	SceneLight::SceneLight(
		const glm::vec3 position,
		const glm::vec3 direction,
		const glm::vec3 color,
		const float dot_inner_limit,
		const float dot_outer_limit) :
		light_type_(SceneLightEnum::SPOT),
		position_(position),
		direction_(direction),
		color_(color),
		dot_inner_limit_(dot_inner_limit),
		dot_outer_limit_(dot_outer_limit) {}

	const glm::mat4 SceneLight::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			SceneNodeInterface::Ptr parent_node = func_(GetParentName());
			if (!parent_node)
			{
				throw std::runtime_error(
					"SceneLight func(" +
					GetParentName() +
					") returned nullptr");
			}
			return parent_node->GetLocalModel(dt);
		}
		return glm::mat4(1.0f);
	}

} // End namespace frame.
