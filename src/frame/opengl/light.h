#pragma once

#include <glm/glm.hpp>

#include <string>

#include "frame/light_interface.h"
#include "frame/opengl/program.h"

namespace frame::opengl
{

/**
 * @class LightPoint
 * @brief Derived from light interface, light point is a point of light in a
 * scene. It radiate in all direction equally.
 */
class LightPoint : public LightInterface
{
  public:
    /**
     * @brief Constructor
     * @param position: position of a light point.
     * @param color_intensity: Color of the light point multiplied by
     * intensity.
     */
    LightPoint(
		const glm::vec3 position,
		const glm::vec3 color_intensity,
		ShadowTypeEnum shadow_type_enum = ShadowTypeEnum::NO_SHADOW)
        : position_(position),
		  color_intensity_(color_intensity),
          shadow_type_enum_(shadow_type_enum)
    {
    }

  public:
    /**
     * @brief Get name.
     * @return Name.
     */
    std::string GetName() const override
    {
		return name_;
	}
    /**
     * @brief Set name.
     * @param name: Name.
     */
    void SetName(const std::string& name) override
    {
        name_ = name;
	}
    /**
     * @brief Get the type of the light, coming from the light interface.
     * @return Return the type of the light.
     */
    const LightTypeEnum GetType() const override
    {
        return LightTypeEnum::POINT_LIGHT;
    }
    /**
     * @brief Get the type of the shadow, coming from the light interface.
     * @return Return the type of the shadow.
     */
    const ShadowTypeEnum GetShadowType() const override
    {
		return shadow_type_enum_;
	}
    /**
     * @brief Get the position of the light, coming from the light
     * interface.
     * @return Return the position of a light.
     */
    const glm::vec3 GetVector() const override
    {
        return position_;
    }
    /**
     * @brief Get the color intensity, coming from the light interface.
     * @return Return the color of the light.
     */
    const glm::vec3 GetColorIntensity() const override
    {
        return color_intensity_;
    }

  protected:
    glm::vec3 position_;
    glm::vec3 color_intensity_;
    ShadowTypeEnum shadow_type_enum_ = ShadowTypeEnum::NO_SHADOW;
	std::string name_;
};

/**
 * @class LightDirectional
 * @brief Derived from light interface, This is a light that come from an
 * infinite position and follow a strait line (like a theoretical sun).
 */
class LightDirectional : public LightInterface
{
  public:
    /**
     * @brief Constructor
     * @param direction: Direction of the light source (normalized).
     * @param color_intensity: Color of the light (technically not
     * containing any intensity).
     */
    LightDirectional(
		const glm::vec3 direction,
		const glm::vec3 color_intensity,
		ShadowTypeEnum shadow_type_enum = ShadowTypeEnum::NO_SHADOW)
        : direction_(direction),
		  color_intensity_(color_intensity),
          shadow_type_enum_(shadow_type_enum)
    {
    }

  public:
    /**
     * @brief Get name.
     * @return Name.
     */
    std::string GetName() const override
    {
        return name_;
    }
    /**
     * @brief Set name.
     * @param name: Name.
     */
    void SetName(const std::string& name) override
    {
        name_ = name;
    }
    /**
     * @brief Get the type of the light, coming from the light interface.
     * @return Return the type of the light.
     */
    const LightTypeEnum GetType() const override
    {
        return LightTypeEnum::DIRECTIONAL_LIGHT;
    }
	/**
     * @brief Get the type of the shadow, coming from the light interface.
     * @return Return the type of the shadow.
     */
	const ShadowTypeEnum GetShadowType() const override
    {
        return shadow_type_enum_;
    }
    /**
     * @brief Get the position of the light, coming from the light
     * interface.
     * @return Return the position of a light.
     */
    const glm::vec3 GetVector() const override
    {
        return direction_;
    }
    /**
     * @brief Get the color intensity, coming from the light interface.
     * @return Return the color of the light.
     */
    const glm::vec3 GetColorIntensity() const override
    {
        return color_intensity_;
    }

  protected:
    glm::vec3 direction_;
    glm::vec3 color_intensity_;
    ShadowTypeEnum shadow_type_enum_ = ShadowTypeEnum::NO_SHADOW;
	std::string name_;
};

/**
 * @class LightManager
 * @brief This is where the light are gathered and assembled to be send to a
 * program.
 */
class LightManager
{
  public:
    /**
     * @brief Add a light to a manager.
     * @param light: Move enable light.
     */
    void AddLight(std::unique_ptr<LightInterface>&& light)
    {
        lights_.push_back(std::move(light));
    }
    //! @brief Remove all light from a manager.
    void RemoveAllLights()
    {
        lights_.clear();
    }
    /**
     * @brief Get light count.
     * @return Return the number of lights.
     */
    const std::size_t GetLightCount() const
    {
        return lights_.size();
    }
    /**
     * @brief Get a light.
     * @param i: Position of a light.
     * @return A pointer to a temporary light.
     */
    const LightInterface* GetLight(int i) const
    {
        return lights_.at(i).get();
    }

  public:
    /**
     * @brief This is where you register light to a program.
     * @param program: A pointer to a temporary program to register light to.
     */
    void RegisterToProgram(Program& program) const;

  protected:
    std::vector<std::unique_ptr<LightInterface>> lights_ = {};
};

} // End namespace frame::opengl.
