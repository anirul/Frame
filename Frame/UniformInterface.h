#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace frame {

/**
 * @class UniformInterface
 * @brief Get access to essential part of the rendering uniform system. This class (and a derived
 * version of it) is to be passed to the rendering system to be able to get the enum uniform.
 */
struct UniformInterface {
    //! @brief Virtual destructor.
    virtual ~UniformInterface() = default;
    /**
     * @brief Get camera position.
     * @return The position of the camera.
     */
    virtual glm::vec3 GetCameraPosition() const = 0;
    /**
     * @brief Get camera front.
     * @return The front normal normal.
     */
    virtual glm::vec3 GetCameraFront() const = 0;
    /**
     * @brief Get camera right.
     * @return The right camera normal.
     */
    virtual glm::vec3 GetCameraRight() const = 0;
    /**
     * @brief Get camera up.
     * @return The up camera normal.
     */
    virtual glm::vec3 GetCameraUp() const = 0;
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
};

}  // End namespace frame.
