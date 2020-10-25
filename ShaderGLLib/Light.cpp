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
			program->Uniform(
				"light_position[" + std::to_string(i) + "]", 
				lights_[i]->GetVector());
			program->Uniform(
				"light_color[" + std::to_string(i) + "]",
				lights_[i]->GetColorIntensity());
			++i;
		}
		program->Uniform("light_max", static_cast<int>(lights_.size()));
	}

} // End namespace sgl.
