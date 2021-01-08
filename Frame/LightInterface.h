#pragma once

#include <cinttypes>
#include <glm/glm.hpp>

namespace frame {

	enum class LightTypeEnum : std::uint8_t
	{
		POINT_LIGHT,
		DIRECTIONAL_LIGHT,
		SPOT_LIGHT,
	};

	struct LightInterface
	{
		virtual const LightTypeEnum GetType() const = 0;
		virtual const glm::vec3 GetVector() const = 0;
		virtual const glm::vec3 GetColorIntensity() const = 0;
	};

} // End namespace frame.
