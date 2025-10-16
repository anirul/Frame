#pragma once

#include <glm/glm.hpp>

namespace frame
{

enum class CameraModeEnum
{
    FREE_ARCBALL,
    Y_AXIS_ALIGNED_ARCBALL
};

class CameraInterface
{
  public:
    virtual ~CameraInterface() = default;
    /**
     * @brief Compute the projection matrix.
     * @return A mat4 of the projection matrix.
     */
    virtual glm::mat4 ComputeProjection() const = 0;
    /**
     * @brief Compute the view matrix.
     * @return A mat4 of the view matrix.
     */
    virtual glm::mat4 ComputeView() const = 0;
    /**
     * @brief Update the front vector (normalized) and update the whole
     *        camera accordingly.
     * @param vec: Front vector of the camera.
     * @return True if the camera is gimbal locked.
     */
    virtual bool SetFront(glm::vec3 vec) = 0;
    /**
     * @brief Set position of the camera and update the whole camera
     *        accordingly.
     * @param vec: Position vector of the camera.
     */
    virtual void SetPosition(glm::vec3 vec) = 0;
    /**
     * @brief Set the up position of the camera and update the whole camera
     * accordingly.
     * @param vec: Up vector of the camera (normalized).
     * @return True if the camera is gimbal locked.
     */
    virtual bool SetUp(glm::vec3 vec) = 0;
    /**
     * @brief Set the field of view in radians.
     * @param fov: Field of view in radians.
     */
    virtual void SetFovRadians(float fov) = 0;
    /**
     * @brief Set the field of view in degrees.
     * @param fov: Field of view in degrees
     */
    virtual void SetFovDegrees(float fov) = 0;
    /**
     * @brief Set aspect ratio (horizontal on vertical).
     * @param aspect_ratio: Aspect ratio of the camera.
     */
    virtual void SetAspectRatio(float aspect_ratio) = 0;
    /**
     * @brief Set aspect ratio (horizontal on vertical).
     * @param aspect_ratio: Aspect ratio of the camera.
     */
    virtual void SetNearClip(float near_clip) = 0;
    /**
     * @brief Set far clipping plane distance.
     * @param far_clip: Set the far boundary of the rendering frustum.
     */
    virtual void SetFarClip(float far_clip) = 0;
    /**
     * @brief Get vertical field of view in radians.
     * @return Vertical field of view in radians.
     */
    virtual float GetFovRadians() const = 0;
    /**
     * @brief Get vertical field of view in degrees.
     * @return Vertical field of view in degrees.
     */
    virtual float GetFovDegrees() const = 0;
    /**
     * @brief Get aspect ration (horizontal on vertical).
     * @return Aspect ration (horizontal on vertical).
     */
    virtual float GetAspectRatio() const = 0;
    /**
     * @brief Get far clipping plane distance.
     * @return Far clipping plane distance.
     */
    virtual float GetFarClip() const = 0;
    /**
     * @brief Get near clipping plane distance.
     * @return Near clipping plane distance.
     */
    virtual float GetNearClip() const = 0;
    /**
     * @brief Get front vector (normalized).
     * @return Front vector (normalized).
     */
    virtual glm::vec3 GetFront() const = 0;
    /**
     * @brief Get position vector.
     * @return Position vector.
     */
    virtual glm::vec3 GetPosition() const = 0;
    /**
     * @brief Get right vector (normalized).
     * @return Right vector (normalized).
     */
    virtual glm::vec3 GetRight() const = 0;
    /**
     * @brief Get up vector (normalized).
     * @return Up vector (normalized).
     */
    virtual glm::vec3 GetUp() const = 0;
    /**
     * @brief Set when you want the camera not to be axis align.
     * @param mode: Set the mode the camera is in, by default this is axis
     * aligned!
     */
    virtual void SetCameraMode(CameraModeEnum camera_mode) = 0;
    /**
     * @brief Get the camera mode.
     * @return The camera mode.
     */
    virtual CameraModeEnum GetCameraMode() const = 0;
};

} // End namespace frame.
