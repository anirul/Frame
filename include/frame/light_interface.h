#pragma once

#include <cinttypes>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "frame/name_interface.h"

namespace frame
{

/**
 * @brief This has to correspond to proto::SceneLight::LightTypeEnum!
 */
enum class LightTypeEnum : std::uint8_t
{
    INVALID_LIGHT		= 0,
    AMBIENT_LIGHT		= 1,
    POINT_LIGHT			= 2,
    DIRECTIONAL_LIGHT	= 3,
    SPOT_LIGHT			= 4,
};

/**
 * @brief This has to correspond to proto::SceneLight::ShadowTypeEnum!
 */
enum class ShadowTypeEnum : std::uint8_t
{
	NO_SHADOW			= 0,
	HARD_SHADOW			= 1,
	SOFT_SHADOW			= 2
};

/**
 * @class LightInterface
 * @brief Light Interface is generic point of light in a scene.
 */
struct LightInterface : public NameInterface
{
    //! @brief Virtual destructor.
    virtual ~LightInterface() = default;
    /**
     * @brief Get the type of the light.
     * @return Return the type of the light.
     */
    virtual LightTypeEnum GetType() const = 0;
    /**
     * @brief Get the type of the shadow.
     * @return Return the type of the shadow.
     */
    virtual ShadowTypeEnum GetShadowType() const = 0;
    /**
     * @brief Get the position of the light or the direction in case this is
     * a directional light.
     * @return Return the position of a light.
     */
    virtual glm::vec3 GetVector() const = 0;
    /**
     * @brief Get the color intensity.
     * @return Return the color of the light.
     */
    virtual glm::vec3 GetColorIntensity() const = 0;
};

} // End namespace frame.
