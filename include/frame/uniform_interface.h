#pragma once

#include <GL/glew.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <memory>
#include "name_interface.h"

namespace frame
{

/**
 * @class UniformInterface
 * @brief An interface to a uniform.
 *
 * This class is an abstractiion of a uniform. It is used to be returned by
 * the uniform collection as a reference to a sisngle uniform.
 */
class UniformInterface : public NameInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~UniformInterface() = default;
    //! @brief Get the name of the uniform.
    virtual std::string GetName() const = 0;
    //! @brief Set the name of the uniform.
    virtual void SetName(const std::string& name) = 0;
    /**
     * @brief Get the type of the uniform.
     * @param value: The type of the uniform (should correspond to the GLenum).
     */
    virtual proto::Uniform::TypeEnum GetType() const = 0;
    /**
     * @brief Set the type of the uniform.
     * @return the type of the uniform.
     */
    virtual void SetType(proto::Uniform::TypeEnum value) = 0;
    /**
     * @brief Get the uniform enum, in case the type enum is INVALID.
     * @return The uniform enum.
     */
    virtual proto::Uniform::UniformEnum GetUniformEnum() const = 0;
    /**
     * @brief Set the uniform enum, type enum is INVALID.
     * @param value: The uniform enum.
     */
    virtual void SetUniformEnum(proto::Uniform::UniformEnum value) = 0;
    /**
     * @brief Get the size of the uniform.
     * @return The size of the uniform.
     */
    virtual glm::uvec2 GetSize() const = 0;
    /**
     * @brief Set the size of the uniform.
     * @param size: The size of the uniform.
     */
    virtual void SetSize(glm::uvec2 size) = 0;
    /**
     * @brief Get the value of the uniform.
     * @return The value of the uniform.
     */
    virtual std::vector<std::int32_t> GetInts() const = 0;
    /**
     * @brief Set the value of the uniform.
     * @param value: The value of the uniform.
     */
    virtual void SetInts(const std::vector<std::int32_t>& value) = 0;
    /**
     * @brief Get the value of the uniform.
     * @return The value of the uniform.
     */
    virtual std::vector<float> GetFloats() const = 0;
    /**
     * @brief Set the value of the uniform.
     * @param value: The value of the uniform.
     */
    virtual void SetFloats(const std::vector<float>& value) = 0;

  public:
    //! @brief Get the values if available.
    virtual float GetFloat() const = 0;
    virtual glm::vec2 GetVec2() const = 0;
    virtual glm::vec3 GetVec3() const = 0;
    virtual glm::vec4 GetVec4() const = 0;
    virtual int GetInt() const = 0;
    virtual glm::ivec2 GetIVec2() const = 0;
    virtual glm::ivec3 GetIVec3() const = 0;
    virtual glm::ivec4 GetIVec4() const = 0;
    virtual glm::mat2 GetMat2() const = 0;
    virtual glm::mat3 GetMat3() const = 0;
    virtual glm::mat4 GetMat4() const = 0;
};

} // End namespace frame.
