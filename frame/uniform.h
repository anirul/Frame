#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "frame/json/proto.h"
#include "frame/serialize.h"
#include "frame/uniform_interface.h"
#include <glm/glm.hpp>

namespace frame
{

/**
 * @class Uniform
 * @brief An interface to a uniform.
 *
 * This class is a uniform. It is used to build a Uniform and return values
 * acording to inputs.
 */
class Uniform : public UniformInterface
{
  public:
    /**
     * @brief Constructors.
     * @param name: Name of the uniform.
     * @param value: The value of the uniform.
     */
    Uniform(const std::string& name, int value);
    Uniform(
        const std::string& name, glm::uvec2 size, const std::vector<int>& list);
    Uniform(const std::string& name, float value);
    Uniform(
        const std::string& name,
        glm::uvec2 size,
        const std::vector<float>& list);
    Uniform(const std::string& name, glm::vec2 value);
    Uniform(const std::string& name, glm::vec3 value);
    Uniform(const std::string& name, glm::vec4 value);
    Uniform(const std::string& name, glm::mat4 value);
    /**
     * @brief Constructor from enum.
     * @param name: Named uniform.
     * @param proto_uniform_enum: The uniform enum.
     */
    Uniform(
        const std::string& name,
        const frame::proto::Uniform::UniformEnum& proto_uniform_enum);
    /**
     * @brief Copy constructor.
     * @param uniform_interface: The uniform interface to copy.
     */
    Uniform(const UniformInterface& uniform_interface);

    /**
     * @brief Get the enum stored in this uniform.
     * @return The uniform enum value.
     */
    proto::Uniform::UniformEnum GetUniformEnum() const
    {
        return data_.uniform_enum();
    }

    /**
     * @brief Set the enum stored in this uniform.
     * @param uniform_enum: The uniform enum value.
     */
    void SetUniformEnum(proto::Uniform::UniformEnum uniform_enum)
    {
        data_.set_uniform_enum(uniform_enum);
    }
};

} // End of namespace frame.
