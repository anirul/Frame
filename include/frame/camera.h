#pragma once

#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace frame
{

enum class CameraModeEnum
{
    FREE_ARCBALL,
    Y_AXIS_ALIGNED_ARCBALL
};

/**
 * @class Camera
 * @brief This is an implementation of a camera class it is done in regard
 *        to computer graphics.
 */
class Camera
{
  public:
    /**
     * @brief Constructor it will create a camera according to the params.
     *
     * You can set the camera mode by using the set camera mode method. Note
     * that in Y_AXIS_ALIGNED_ARCBALL you won't be able to modify the up
     * vector by the SetUp() method!
     *
     * @param position: Position of the camera.
     * @param front: Direction the camera is facing to (normalized).
     * @param up: Direction of the up for the camera (normalized).
     * @param fov_degrees: Field of view (angle in degrees of the vertical
     *        field of view).
     * @param aspect_ratio: Aspect ratio of the screen (weight on height).
     * @param near_clip: Near clipping plane (front distance to be drawn).
     * @param far_clip: Far clipping plane (back distance to be drawn).
     * @param y_align: Used for arc ball should be true.
     */
    Camera(
        glm::vec3 position = {0.f, 0.f, 0.f},
        glm::vec3 front = {0.f, 0.f, -1.f},
        glm::vec3 up = {0.f, 1.f, 0.f},
        float fov_degrees = 65.0f,
        float aspect_ratio = 16.0f / 9.0f,
        float near_clip = 0.1f,
        float far_clip = 1000.0f,
        CameraModeEnum camera_mode = CameraModeEnum::Y_AXIS_ALIGNED_ARCBALL);
    /**
     * @brief Copy constructor.
     * @param camera: The camera from which this one will be created.
     */
    Camera(const Camera& camera);
    /**
     * @brief Copy assignment operator.
     * @param camera: The camera from which this one will be created.
     * @return The camera that was created.
     */
    Camera& operator=(const Camera& camera);

  public:
    /**
     * @brief Compute the view matrix.
     * @return A mat4 of the view matrix.
     */
    glm::mat4 ComputeView() const;
    /**
     * @brief Compute the projection matrix.
     * @return A mat4 of the projection matrix.
     */
    glm::mat4 ComputeProjection() const;
    /**
     * @brief Update the front vector (normalized) and update the whole
     *        camera accordingly.
     * @param vec: Front vector of the camera.
     * @return True if the camera is gimbal locked.
     */
    bool SetFront(glm::vec3 vec);
    /**
     * @brief Set position of the camera and update the whole camera
     *        accordingly.
     * @param vec: Position vector of the camera.
     */
    void SetPosition(glm::vec3 vec);
    /**
     * @brief Set the up position of the camera and update the whole camera
     * accordingly.
     * @param vec: Up vector of the camera (normalized).
     * @return True if the camera is gimbal locked.
     */
    bool SetUp(glm::vec3 vec);

  public:
    /**
     * @brief Set the field of view in radians.
     * @param fov: Field of view in radians.
     */
    void SetFovRadians(float fov)
    {
        fov_rad_ = fov;
    }
    /**
     * @brief Set the field of view in degrees.
     * @param fov: Field of view in degrees
     */
    void SetFovDegrees(float fov)
    {
        fov_rad_ = glm::radians(fov);
    }
    /**
     * @brief Set aspect ratio (horizontal on vertical).
     * @param aspect_ratio: Aspect ratio of the camera.
     */
    void SetAspectRatio(float aspect_ratio)
    {
        aspect_ratio_ = aspect_ratio;
    }
    /**
     * @brief Set near clipping plane distance.
     * @param near_clip: Set the boundary of the close rendering frustum.
     */
    void SetNearClip(float near_clip)
    {
        near_clip_ = near_clip;
    }
    /**
     * @brief Set far clipping plane distance.
     * @param far_clip: Set the far boundary of the rendering frustum.
     */
    void SetFarClip(float far_clip)
    {
        far_clip_ = far_clip;
    }
    /**
     * @brief Get front vector (normalized).
     * @return Front vector (normalized).
     */
    glm::vec3 GetFront() const
    {
        return front_;
    }
    /**
     * @brief Get position vector.
     * @return Position vector.
     */
    glm::vec3 GetPosition() const
    {
        return position_;
    }
    /**
     * @brief Get right vector (normalized).
     * @return Right vector (normalized).
     */
    glm::vec3 GetRight() const
    {
        return right_;
    }
    /**
     * @brief Get up vector (normalized).
     * @return Up vector (normalized).
     */
    glm::vec3 GetUp() const
    {
        return up_;
    }
    /**
     * @brief Get vertical field of view in radians.
     * @return Vertical field of view in radians.
     */
    float GetFovRadians() const
    {
        return fov_rad_;
    }
    /**
     * @brief Get vertical field of view in degrees.
     * @return Vertical field of view in degrees.
     */
    float GetFovDegrees() const
    {
        return glm::degrees(fov_rad_);
    }
    /**
     * @brief Get aspect ration (horizontal on vertical).
     * @return Aspect ration (horizontal on vertical).
     */
    float GetAspectRatio() const
    {
        return aspect_ratio_;
    }
    /**
     * @brief Get near clipping plane distance.
     * @return Near clipping plane distance.
     */
    float GetNearClip() const
    {
        return near_clip_;
    }
    /**
     * @brief Get far clipping plane distance.
     * @return Far clipping plane distance.
     */
    float GetFarClip() const
    {
        return far_clip_;
    }
    /**
     * @brief Set when you want the camera not to be axis align.
     * @param mode: Set the mode the camera is in, by default this is axis
     * aligned!
     */
    void SetCameraMode(CameraModeEnum camera_mode)
    {
        camera_mode_ = camera_mode;
    }
    /**
     * @brief Get the camera mode.
     * @return The camera mode.
     */
    CameraModeEnum GetCameraMode() const
    {
        return camera_mode_;
    }

  protected:
    /**
     * @brief This will update all the vector by using cross product.
     * @return True if the camera is now gimbal locked.
     */
    bool UpdateCameraVectors();

  private:
    // Default values for each parameters.
    glm::vec3 position_ = {0, 0, 0};
    glm::vec3 front_ = {0, 0, -1};
    glm::vec3 up_ = {0, 1, 0};
    glm::vec3 right_ = {1, 0, 0};
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    float fov_rad_ = glm::radians(65.0f);
    float aspect_ratio_ = 16.f / 9.f;
    float near_clip_ = 0.1f;
    float far_clip_ = 1000.0f;
    CameraModeEnum camera_mode_ = CameraModeEnum::Y_AXIS_ALIGNED_ARCBALL;
};

} // End namespace frame.
