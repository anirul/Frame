#include "Light.h"
#include <stdexcept>

namespace sgl {

	void LightManager::RegisterToProgram(
		const std::shared_ptr<Program>& program) const
	{
		if (lights_.size() > 32)
		{
			throw std::runtime_error("too many lights!");
		}
		program->Use();
		int i = 0;
		for (const auto& light : lights_)
		{
			program->UniformVector3(
				"light_position[" + std::to_string(i) + "]", 
				lights_[i]->GetVector());
			program->UniformVector3(
				"light_color[" + std::to_string(i) + "]",
				lights_[i]->GetColorIntensity());
			++i;
		}
		program->UniformInt("light_max", static_cast<int>(lights_.size()));
	}

} // End namespace sgl.
