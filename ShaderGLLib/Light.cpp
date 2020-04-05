#include "Light.h"
#include <stdexcept>

namespace sgl {

	sgl::LightManager& LightManager::operator=(
		const LightManager& light_manager)
	{
		lights_ = light_manager.lights_;
		return *this;
	}

	void LightManager::RegisterToProgram(
		const std::shared_ptr<Program>& program)
	{
		if (lights_.size() > max_dynamic_lights_)
		{
			throw std::runtime_error("too many lights.");
		}
		int i = 0;
		// Fill correct lights.
		for (sgl::Light light : lights_)
		{
			program->UniformVector3(
				"light_position[" + std::to_string(i) + "]", 
				light.GetPosition());
			program->UniformVector3(
				"light_color[" + std::to_string(i) + "]",
				light.GetColorIntensity());
			++i;
		}
		sgl::Light light_zero(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0));
		// Fill what is left with empty ones.
		for (; i < max_dynamic_lights_; ++i)
		{
			program->UniformVector3(
				"light_position[" + std::to_string(i) + "]",
				light_zero.GetPosition());
			program->UniformVector3(
				"light_color[" + std::to_string(i) + "]",
				light_zero.GetColorIntensity());
		}
	}

} // End namespace sgl.
