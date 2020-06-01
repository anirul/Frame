#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Program.h"

namespace sgl {

	enum class light_type
	{
		POINT_LIGHT,
		DIRECTIONAL_LIGHT,
		SPOT_LIGHT,
	};

	class LightInterface
	{
	public:
		virtual const light_type GetType() const = 0;
		virtual const glm::vec3 GetVector() const = 0;
		virtual const glm::vec3 GetColorIntensity() const = 0;
	};

	class LightPoint : public LightInterface
	{
	public:
		LightPoint(
			const glm::vec3& position, 
			const glm::vec3& color_intensity) :
			position_(position), 
			color_intensity_(color_intensity) {}

	public:
		const light_type GetType() const override 
		{ 
			return light_type::POINT_LIGHT; 
		}
		const glm::vec3 GetVector() const override { return position_; }
		const glm::vec3 GetColorIntensity() const override 
		{ 
			return color_intensity_; 
		}

	protected:
		glm::vec3 position_;
		glm::vec3 color_intensity_;
	};

	class LightDirectional : public LightInterface
	{
	public:
		LightDirectional(
			const glm::vec3& direction,
			const glm::vec3& color_intensity) :
			direction_(direction),
			color_intensity_(color_intensity) {}

	public:
		const light_type GetType() const override
		{
			return light_type::DIRECTIONAL_LIGHT;
		}
		const glm::vec3 GetVector() const override { return direction_; }
		const glm::vec3 GetColorIntensity() const override
		{
			return color_intensity_;
		}

	protected:
		glm::vec3 direction_;
		glm::vec3 color_intensity_;
	};

	class LightManager 
	{
	public:
		LightManager() = default;
		LightManager& operator=(const LightManager& light_manager) = default;

	public:
		void AddLight(const std::shared_ptr<LightInterface>& light) 
		{ 
			lights_.push_back(light); 
		}
		void RemoveAllLights() { lights_.clear(); }
		const std::size_t GetLightCount() const { return lights_.size(); }
		const std::shared_ptr<LightInterface>& GetLight(int i) const 
		{ 
			return lights_.at(i);
		}

	public:
		void RegisterToProgram(
			const std::shared_ptr<Program>& program) const;

	protected:
		std::vector<std::shared_ptr<LightInterface>> lights_;
	};

} // End namespace sgl.
