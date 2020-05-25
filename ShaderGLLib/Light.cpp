#include "Light.h"
#include <stdexcept>

namespace sgl {

	void LightManager::RegisterToProgram(
		const std::shared_ptr<Program>& program,
		const int position) const
	{
		if (position > lights_.size())
		{
			throw std::runtime_error("Light is out of range.");
		}
		program->Use();
		program->UniformVector3(
			"light_position", 
			lights_[position]->GetVector());
		program->UniformVector3(
			"light_color",
			lights_[position]->GetColorIntensity());
	}

} // End namespace sgl.
