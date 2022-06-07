#pragma once

#include "frame/level_interface.h"
#include "frame/uniform_interface.h"

namespace frame {

/**
 * @class UniformWrapper
 * @brief Get access to essential part of the rendering uniform system. This class is to be passed
 * to the rendering system to be able to get the enum uniform.
 */
class UniformWrapper : public UniformInterface {
   public:
    /**
     * @brief Constructor create a wrapper from a camera pointer.
	 * @param Camera pointer.
     */
    UniformWrapper(Camera* camera) { camera_ = camera; }

   public:
    /**
     * @brief Set the projection matrix.
     * @param projection: The projection matrix.
     */
    void SetProjection(glm::mat4 projection) { projection_ = projection; }
    /**
     * @brief Set the view matrix.
     * @param view: The view matrix.
     */
    void SetView(glm::mat4 view) { view_ = view; }
    /**
     * @brief Set the model matrix.
     * @param model: The model matrix.
     */
    void SetModel(glm::mat4 model) { model_ = model; }
    /**
     * @brief Set the delta time.
     * @param time: The delta time.
     */
    void SetTime(double time) { time_ = time; }

   public:
    /**
     * @brief Get camera position.
     * @return The position of the camera.
     */
    glm::vec3 GetCameraPosition() const override;
    /**
     * @brief Get camera front.
     * @return The front normal normal.
     */
    glm::vec3 GetCameraFront() const override;
    /**
     * @brief Get camera right.
     * @return The right camera normal.
     */
    glm::vec3 GetCameraRight() const override;
    /**
     * @brief Get camera up.
     * @return The up camera normal.
     */
    glm::vec3 GetCameraUp() const override;
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
     * @brief Get the delta time.
     * @return The delta time.
     */
    double GetDeltaTime() const override;

   private:
    Camera* camera_       = nullptr;
    glm::mat4 model_      = glm::mat4(1.0f);
    glm::mat4 projection_ = glm::mat4(1.0f);
    glm::mat4 view_       = glm::mat4(1.0f);
    double time_          = 0.0;
};

}  // End namespace frame.
