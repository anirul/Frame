#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Program.h"

namespace sgl {

	class Light 
	{
	public:
		Light(const glm::vec3& position, const glm::vec3 color_intensity) :
			position_(position), color_intensity_(color_intensity) {}

	public:
		const glm::vec3 GetPosition() const { return position_; }
		const glm::vec3 GetColorIntensity() const { return color_intensity_; }

	protected:
		glm::vec3 position_;
		glm::vec3 color_intensity_;
	};

	class LightManager 
	{
	public:
		LightManager() = default;

	public:
		LightManager& operator=(const LightManager& light_manager);
		void AddLight(const Light& light) { lights_.push_back(light); }
		void RemoveAllLights() { lights_.clear(); }
		const std::size_t GetLightCount() const { return lights_.size(); }

	public:
		void RegisterToProgram(const std::shared_ptr<Program>& program);

	protected:
		const int max_dynamic_lights_ = 4;
		std::vector<Light> lights_;
	};

} // End namespace sgl.
