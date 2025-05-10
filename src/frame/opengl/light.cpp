#include "light.h"

#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "frame/uniform.h"

namespace frame::opengl
{

void LightManager::RegisterToProgram(Program& program) const
{
    if (lights_.size() > 32)
    {
        throw std::runtime_error("too many lights!");
    }
    program.Use();
    int i = 0;
    for (const auto& light : lights_)
    {
        std::unique_ptr<UniformInterface> position_interface =
            std::make_unique<Uniform>(
                "light_position[" + std::to_string(i) + "]",
                light->GetVector());
        program.AddUniform(std::move(position_interface));
        std::unique_ptr<UniformInterface> color_interface =
            std::make_unique<Uniform>(
                "light_direction[" + std::to_string(i) + "]",
                light->GetColorIntensity());
        program.AddUniform(std::move(color_interface));
        ++i;
    }
    std::unique_ptr<UniformInterface> light_max_interface =
        std::make_unique<Uniform>(
            "light_max", static_cast<int>(lights_.size()));
    program.AddUniform(std::move(light_max_interface));
    program.UnUse();
}

} // End namespace frame::opengl.
