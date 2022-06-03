#pragma once

#include <glm/glm.hpp>
#include <utility>

namespace frame {

/**
 * @class Camera
 * @brief This is an implementation of a camera class it is done in regard to computer graphics.
 */
class Camera {
   public:
    /**
	* @brief Constructor it will create a camera acording to the params.
	* @param position: Position of the camera.
	* @param front: Direction the camera is facing to (normalized).
	* @param up: Direction of the up for the camera (normalized).
	* @param fov_degrees: Field of view (angle in degrees of the vertical field of view).
	* @param aspect_ratio: Aspect ratio of the screen weigth on height.
	* @param near_clip: Near clipping plane (front distance to be drawn).
	* @param far_clip: Far clipping plane (back distance to be drawn).
	*/
    Camera(glm::vec3 position = { 0.f, 0.f, 0.f }, glm::vec3 front = { 0.f, 0.f, -1.f },
           glm::vec3 up = { 0.f, 1.f, 0.f }, float fov_degrees = 65.0f,
           float aspect_ratio = 16.0f / 9.0f, float near_clip = 0.1f, float far_clip = 1000.0f);

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
	* @brief Update the front vector (normalized) and update the whole camera accordingly.
	* @param vec: Front vector of the camera.
	*/
    void SetFront(glm::vec3 vec);
    /**
	* @brief Set position of the camera and update the whole camera accordingly.
	* @param vec: Position vector of the camera.
	*/
    void SetPosition(glm::vec3 vec);
    /**
	* @brief Set the up position of the camera and update the whole camera accordingly.
	* @param vec Up vector of the camera (normalized).
	*/
    void SetUp(glm::vec3 vec);

   public:
    /**
	* @brief Set the field of view in radians.
	* @param fov: Field of view in radians.
	*/
    void SetFovRadians(float fov) { fov_rad_ = fov; }
    /**
	* @brief Set the field of view in degrees.
	* @param fov: Field of view in degrees
	*/
    void SetFovDegrees(float fov) { fov_rad_ = glm::radians(fov); }
    /**
	* @brief Set aspect ratio (horizontal on vertical).
	* @param aspect_ratio: Aspect ratio of the camera.
	*/
    void SetAspectRatio(float aspect_ratio) { aspect_ratio_ = aspect_ratio; }
    /**
	* @brief Set near clipping plane distance.
	* @param near_clip: Set the boundary of the close rendering frustrum.
	*/
    void SetNearClip(float near_clip) { near_clip_ = near_clip; }
    /**
	* @brief Set far clipping plane distance.
	* @param far_clip: Set the far boundery of the rendering frustrum.
	*/
    void SetFarClip(float far_clip) { far_clip_ = far_clip; }
    glm::vec3 GetFront() const { return front_; }
    glm::vec3 GetPosition() const { return position_; }
    glm::vec3 GetRight() const { return right_; }
    glm::vec3 GetUp() const { return up_; }
    float GetFovRadians() const { return fov_rad_; }
    float GetFovDegrees() const { return glm::degrees(fov_rad_); }
    float GetAspectRatio() const { return aspect_ratio_; }
    float GetNearClip() const { return near_clip_; }
    float GetFarClip() const { return far_clip_; }

   protected:
    void UpdateCameraVectors();
    glm::vec3 position_ = { 0, 0, 0 };
    glm::vec3 front_    = { 0, 0, -1 };
    glm::vec3 up_       = { 0, 1, 0 };
    glm::vec3 right_    = { 1, 0, 0 };
    float yaw_          = -90.0f;
    float pitch_        = 0.0f;
    float fov_rad_      = glm::radians(65.0f);
    float aspect_ratio_ = 16.f / 9.f;
    float near_clip_    = 0.1f;
    float far_clip_     = 1000.0f;
};

}  // End namespace frame.
