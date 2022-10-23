#include "frame/camera.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace frame {

Camera::Camera(glm::vec3 position /*= { 0.f, 0.f, 0.f }*/, glm::vec3 front /*= { 0.f, 0.f, 1.f }*/,
               glm::vec3 up /*= { 0.f, 1.f, 0.f }*/, float fov_degrees /*= 65.0f */,
               float aspect_ratio /*= 16.0f / 9.0f*/, float near_clip /*= 0.1f*/,
               float far_clip /*= 1000.0f*/)
    : position_(position),
      front_(front),
      up_(up),
      fov_rad_(glm::radians(fov_degrees)),
      aspect_ratio_(aspect_ratio),
      near_clip_(near_clip),
      far_clip_(far_clip) {
    UpdateCameraVectors();
}

Camera::Camera(const Camera& camera) {
    position_     = camera.position_;
    front_        = camera.front_;
    up_           = camera.up_;
    fov_rad_      = camera.fov_rad_;
    aspect_ratio_ = camera.aspect_ratio_;
    near_clip_    = camera.near_clip_;
    far_clip_     = camera.far_clip_;
    UpdateCameraVectors();
}

frame::Camera& Camera::operator=(const Camera& camera) {
    position_     = camera.position_;
    front_        = camera.front_;
    up_           = camera.up_;
    fov_rad_      = camera.fov_rad_;
    aspect_ratio_ = camera.aspect_ratio_;
    near_clip_    = camera.near_clip_;
    far_clip_     = camera.far_clip_;
    UpdateCameraVectors();
    return *this;
}

glm::mat4 Camera::ComputeView() const { return glm::lookAt(position_, front_ + position_, up_); }

glm::mat4 Camera::ComputeProjection() const {
    return glm::perspective(fov_rad_, aspect_ratio_, near_clip_, far_clip_);
}

void Camera::SetFront(glm::vec3 vec) {
    front_ = vec;
    UpdateCameraVectors();
}

void Camera::SetPosition(glm::vec3 vec) {
    position_ = vec;
    UpdateCameraVectors();
}

void Camera::SetUp(glm::vec3 vec) {
    up_ = vec;
    UpdateCameraVectors();
}

void Camera::SetRight(glm::vec3 vec) {
    right_ = vec;
    up_    = glm::normalize(glm::cross(front_, -right_));
    UpdateCameraVectors();
}

void Camera::UpdateCameraVectors() {
    // TODO(anirul): Check field of view for correctness.
    front_ = glm::normalize(front_);
    right_ = glm::normalize(glm::cross(front_, up_));
    up_    = glm::normalize(glm::cross(right_, front_));
}

}  // End namespace frame.
