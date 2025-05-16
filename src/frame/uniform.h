#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "frame/json/proto.h"
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

  public:
    //! @brief Get the name of the uniform.
    std::string GetName() const override;
    //! @brief Set the name of the uniform.
    void SetName(const std::string& name) override;
    /**
     * @brief Get the type of the uniform.
     * @param value: The type of the uniform (should correspond to the GLenum).
     */
    proto::Uniform::TypeEnum GetType() const override;
    /**
     * @brief Set the type of the uniform.
     * @return the type of the uniform.
     */
    void SetType(proto::Uniform::TypeEnum value) override;
    /**
     * @brief Get the uniform enum, in case the type enum is INVALID.
     * @return The uniform enum.
     */
    proto::Uniform::UniformEnum GetUniformEnum() const override;
    /**
     * @brief Set the uniform enum, type enum is INVALID.
     * @param value: The uniform enum.
     */
    void SetUniformEnum(proto::Uniform::UniformEnum value) override;
    /**
     * @brief Get the size of the uniform.
     * @return The size of the uniform.
     */
    glm::uvec2 GetSize() const override;
    /**
     * @brief Set the size of the uniform.
     * @param size: The size of the uniform.
     */
    void SetSize(glm::uvec2 size) override;
    /**
     * @brief Get the value of the uniform.
     * @return The value of the uniform.
     */
    std::vector<std::int32_t> GetInts() const override;
    /**
     * @brief Set the value of the uniform.
     * @param value: The value of the uniform.
     */
    void SetInts(const std::vector<std::int32_t>& value) override;
    /**
     * @brief Get the value of the uniform.
     * @return The value of the uniform.
     */
    std::vector<float> GetFloats() const override;
    /**
     * @brief Set the value of the uniform.
     * @param value: The value of the uniform.
     */
    void SetFloats(const std::vector<float>& value) override;

  public:
    //! @brief Get the values if available.
    float GetFloat() const override;
    glm::vec2 GetVec2() const override;
    glm::vec3 GetVec3() const override;
    glm::vec4 GetVec4() const override;
    int GetInt() const override;
    glm::mat4 GetMat4() const override;

  protected:
    std::string name_ = "";
    proto::Uniform::TypeEnum type_ = proto::Uniform::INVALID_TYPE;
    proto::Uniform::UniformEnum uniform_enum_ = proto::Uniform::INVALID_UNIFORM;
    glm::uvec2 size_ = {1, 1};
    std::vector<float> value_float_ = {};
    std::vector<std::int32_t> value_int_ = {};
};

} // End of namespace frame.
