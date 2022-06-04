#pragma once

#include <cinttypes>
#include <glm/glm.hpp>

namespace frame {

enum class LightTypeEnum : std::uint8_t {
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    SPOT_LIGHT,
};

/**
 * @class LightInterface
 * @brief Light Interface is generic point of light in a scene.
 */
struct LightInterface {
    //! @brief Virtual destructor.
    virtual ~LightInterface()                         = default;
    /**
     * @brief Get the type of the light.
     * @return Return the type of the light.
     */
    virtual const LightTypeEnum GetType() const       = 0;
    /**
     * @brief Get the position of the light or the direction in case this is a directional light.
     * @return Return the position of a light.
     */
    virtual const glm::vec3 GetVector() const         = 0;
    /**
     * @brief Get the color intensity.
     * @return Return the color of the light.
     */
    virtual const glm::vec3 GetColorIntensity() const = 0;
};

}  // End namespace frame.
