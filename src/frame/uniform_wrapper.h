#pragma once

#include "frame/level_interface.h"
#include "frame/uniform_interface.h"

namespace frame
{

/**
 * @class UniformWrapper
 * @brief Get access to essential part of the rendering uniform system.
 *
 * This class is to be passed to the rendering system to be able to get the
 * enum uniform.
 */
class UniformWrapper : public UniformInterface
{
  public:
    /**
     * @brief Default constructor.
     */
    UniformWrapper() = default;
    /**
     * @brief Constructor create a wrapper from a camera pointer.
     * @param Camera pointer.
     */
    UniformWrapper(
        const glm::mat4 &projection,
        const glm::mat4 &view,
        const glm::mat4 &model,
        double dt);

  public:
    /**
     * @brief Set the projection matrix.
     * @param projection: The projection matrix.
     */
    void SetProjection(glm::mat4 projection)
    {
        projection_ = projection;
    }
    /**
     * @brief Set the view matrix.
     * @param view: The view matrix.
     */
    void SetView(glm::mat4 view)
    {
        view_ = view;
    }
    /**
     * @brief Set the model matrix.
     * @param model: The model matrix.
     */
    void SetModel(glm::mat4 model)
    {
        model_ = model;
    }
    /**
     * @brief Set the delta time.
     * @param time: The delta time.
     */
    void SetTime(double time)
    {
        time_ = time;
    }

  public:
    /**
     * @brief Get the projection matrix.
     * @return The projection matrix.
     */
    glm::mat4 GetProjection() const override;
    /**
     * @brief Get the view matrix.
     * @return The view matrix.
     */
    glm::mat4 GetView() const override;
    /**
     * @brief Get the model matrix.
     * @return The model matrix.
     */
    glm::mat4 GetModel() const override;
    /**
     * @brief Set value float.
     * @param name: Connection name.
     * @param vector: Values.
     * @param size: The size of the vector.
     */
    void SetValueFloat(
        const std::string &name,
        const std::vector<float> &vector,
        glm::uvec2 size) override;
    /**
     * @brief Set value int.
     * @param name: Connection name.
     * @param vector: Values.
     * @param size: The size of the vector.
     */
    void SetValueInt(
        const std::string &name,
        const std::vector<std::int32_t> &vector,
        glm::uvec2 size) override;
    /**
     * @brief Get the value of a stream.
     * @param name: Name of the stream.
     * @return The vector that correspond to the value of a stream.
     */
    std::vector<float> GetValueFloat(const std::string &name) const override;
    /**
     * @brief Get the value of a stream.
     * @param name: Name of the stream.
     * @return The vector that correspond to the value of a stream.
     */
    std::vector<std::int32_t> GetValueInt(
        const std::string &name) const override;
    /**
     * @brief Get a list of names for the float uniform plugin.
     * @return The list of names for the float uniform plugin.
     */
    std::vector<std::string> GetFloatNames() const override;
    /**
     * @brief Get a list of names for the int uniform plugin.
     * @return The list of names for the int uniform plugin.
     */
    std::vector<std::string> GetIntNames() const override;
    /**
     * @brief Get the value of a stream.
     * @param name: Name of the stream.
     * @return The vector that correspond to the value of a stream.
     */
    glm::uvec2 GetSizeFromFloat(const std::string &name) const override;
    /**
     * @brief Get the value of a stream.
     * @param name: Name of the stream.
     * @return The vector that correspond to the value of a stream.
     */
    glm::uvec2 GetSizeFromInt(const std::string &name) const override;
    /**
     * @brief Get the delta time.
     * @return The delta time.
     */
    double GetDeltaTime() const override;

  private:
    struct FloatValues
    {
        std::vector<float> value;
        glm::uvec2 size;
    };
    struct IntValues
    {
        std::vector<std::int32_t> value;
        glm::uvec2 size;
    };
    glm::mat4 model_ = glm::mat4(1.0f);
    glm::mat4 projection_ = glm::mat4(1.0f);
    glm::mat4 view_ = glm::mat4(1.0f);
    std::map<std::string, FloatValues> stream_value_float_map_ = {};
    std::map<std::string, IntValues> stream_value_int_map_ = {};
    double time_ = 0.0;
};

} // End namespace frame.
