#include "frame/uniform_wrapper.h"

#include "frame/node_matrix.h"

namespace frame {

glm::vec3 UniformWrapper::GetCameraPosition() const {
    if (camera_) return camera_->GetPosition();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraFront() const {
    if (camera_) return camera_->GetFront();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraRight() const {
    if (camera_) return camera_->GetRight();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraUp() const {
    if (camera_) return camera_->GetUp();
    return glm::vec3(0.0f);
}

glm::mat4 UniformWrapper::GetProjection() const {
    if (camera_) return camera_->ComputeProjection();
    return projection_;
}

glm::mat4 UniformWrapper::GetView() const {
    if (camera_) return camera_->ComputeView();
    return view_;
}

glm::mat4 UniformWrapper::GetModel() const { return model_; }

double UniformWrapper::GetDeltaTime() const { return time_; }

}  // End namespace frame.
