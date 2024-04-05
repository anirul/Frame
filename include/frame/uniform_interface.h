#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <memory>

namespace frame
{

/**
 * @class UniformInterface
 * @brief Get access to essential part of the rendering uniform system.
 *
 * This class (and a derived version of it) is to be passed to the rendering
 * system to be able to get the enum uniform.
 */
class UniformInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~UniformInterface() = default;
    /**
     * @brief Get the projection matrix.
     * @return The projection matrix.
     */
    virtual glm::mat4 GetProjection() const = 0;
    /**
     * @brief Get the view matrix.
     * @return The view matrix.
     */
    virtual glm::mat4 GetView() const = 0;
    /**
     * @brief Get the model matrix.
     * @return The model matrix.
     */
    virtual glm::mat4 GetModel() const = 0;
    /**
     * @brief Get the delta time.
     * @return The delta time.
     */
    virtual double GetDeltaTime() const = 0;
    /**
     * @brief Set value float.
     * @param name: Connection name.
     * @param vector: Values.
     * @param size: The size of the vector.
     */
    virtual void SetValueFloat(
        const std::string& name,
        const std::vector<float>& vector,
        glm::uvec2 size) = 0;
    /**
     * @brief Set value int.
     * @param name: Connection name.
     * @param vector: Values.
     * @param size: The size of the vector.
     */
    virtual void SetValueInt(
        const std::string& name,
        const std::vector<std::int32_t>& vector,
        glm::uvec2 size) = 0;
    /**
     * @brief Get a list of names for the float uniform plugin.
     * @return The list of names for the float uniform plugin.
     */
    virtual std::vector<std::string> GetFloatNames() const = 0;
    /**
     * @brief Get a list of names for the int uniform plugin.
     * @return The list of names for the int uniform plugin.
     */
    virtual std::vector<std::string> GetIntNames() const = 0;
    /**
     * @brief Get a value.
     * @param name: Connection name.
     * @return The vector that correspond to the value.
     */
    virtual std::vector<float> GetValueFloat(const std::string& name) const = 0;
    /**
     * @brief Get a value.
     * @param name: Connection name.
     * @return The vector that correspond to the value.
     */
    virtual std::vector<std::int32_t> GetValueInt(
        const std::string& name) const = 0;
    /**
     * @brief Get the size of a value.
     * @param name: Connection name.
     * @return The size of value.
     */
    virtual glm::uvec2 GetSizeFromFloat(const std::string& name) const = 0;
    /**
     * @brief Get the size of a value.
     * @param name: Connection name.
     * @return The size of value.
     */
    virtual glm::uvec2 GetSizeFromInt(const std::string& name) const = 0;
};

} // End namespace frame.
