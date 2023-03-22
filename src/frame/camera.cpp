#include "frame/camera.h"

#include <frame/logger.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <set>

namespace frame {

Camera::Camera(glm::vec3 position /*= { 0.f, 0.f, 0.f }*/, glm::vec3 front /*= { 0.f, 0.f, -1.f }*/,
               glm::vec3 up /*= { 0.f, 1.f, 0.f }*/, float fov_degrees /*= 65.0f */,
               float aspect_ratio /*= 16.0f / 9.0f*/, float near_clip /*= 0.1f*/,
               float far_clip /*= 1000.0f*/,
               CameraModeEnum camera_mode /*= CameraModeEnum::Y_AXIS_ALIGNED_ARCBALL*/)
    : position_(position),
      front_(front),
      up_(up),
      fov_rad_(glm::radians(fov_degrees)),
      aspect_ratio_(aspect_ratio),
      near_clip_(near_clip),
      far_clip_(far_clip),
      camera_mode_(camera_mode) {
    UpdateCameraVectors();
}

Camera::Camera(const Camera& camera) {
    SetCameraMode(camera.GetCameraMode());
    SetFovRadians(camera.GetFovRadians());
    SetAspectRatio(camera.GetAspectRatio());
    SetNearClip(camera.GetNearClip());
    SetFarClip(camera.GetFarClip());
    SetPosition(camera.GetPosition());
    SetFront(camera.GetFront());
    SetUp(camera.GetUp());
    UpdateCameraVectors();
}

frame::Camera& Camera::operator=(const Camera& camera) {
    SetCameraMode(camera.GetCameraMode());
    SetFovRadians(camera.GetFovRadians());
    SetAspectRatio(camera.GetAspectRatio());
    SetNearClip(camera.GetNearClip());
    SetFarClip(camera.GetFarClip());
    SetPosition(camera.GetPosition());
    SetFront(camera.GetFront());
    SetUp(camera.GetUp());
    UpdateCameraVectors();
    return *this;
}

glm::mat4 Camera::ComputeView() const { return glm::lookAt(position_, front_ + position_, up_); }

glm::mat4 Camera::ComputeProjection() const {
    return glm::perspective(fov_rad_, aspect_ratio_, near_clip_, far_clip_);
}

void Camera::SetPosition(glm::vec3 vec) { position_ = vec; }

bool Camera::SetFront(glm::vec3 vec) {
    front_ = glm::normalize(vec);
    return UpdateCameraVectors();
}

bool Camera::SetUp(glm::vec3 vec) {
    up_ = glm::normalize(vec);
    return UpdateCameraVectors();
}

bool Camera::UpdateCameraVectors() {
    constexpr glm::vec3 UP(0, 1, 0);
    front_ = glm::normalize(front_);
    if (camera_mode_ == CameraModeEnum::Y_AXIS_ALIGNED_ARCBALL) {
        auto previous_right = right_;
        auto previous_up    = up_;
        right_              = glm::normalize(glm::cross(front_, UP));
        up_                 = glm::normalize(glm::cross(right_, front_));
        if (std::abs(glm::dot(up_, UP)) < 0.1) {
            right_ = previous_right;
            up_    = previous_up;
            front_ = glm::normalize(glm::cross(up_, right_));
            return true;
        }
    } else {
        right_ = glm::normalize(glm::cross(front_, up_));
        up_    = glm::normalize(glm::cross(right_, front_));
    }
    return false;
}

}  // End namespace frame.
