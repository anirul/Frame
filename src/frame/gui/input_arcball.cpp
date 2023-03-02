#include "frame/gui/input_arcball.h"

#include <glm/gtc/matrix_transform.hpp>

#include "glm/gtx/quaternion.hpp"

namespace {
const float pi = std::acos(-1.0f);
}

namespace frame::gui {

bool InputArcball::KeyPressed(char key, double dt) { return true; }

bool InputArcball::KeyReleased(char key, double dt) { return true; }

bool InputArcball::MouseMoved(glm::vec2 position, glm::vec2 relative, double dt) {
    if (!mouse_active_) return true;
    auto window_size = device_.GetSize();
    auto& camera     = device_.GetLevel().GetDefaultCamera();
    glm::vec4 cam_position = glm::vec4(camera.GetPosition(), 1.0f);
    glm::vec4 cam_front    = glm::normalize(glm::vec4(camera.GetFront(), 1.0f));
    glm::vec2 delta_angle(2.0f * pi / window_size.x, pi / window_size.y);
    glm::vec2 angle_xy(relative.x * delta_angle.x * move_multiplication_,
                       relative.y * delta_angle.y * move_multiplication_);
    // Rotate the camera around the pivot on the X axis.
    glm::mat4 rotation_x(1.0f);
    rotation_x   = glm::rotate(rotation_x, angle_xy.x, camera.GetUp());
    cam_position = rotation_x * (cam_position - pivot_) + pivot_;
    // Rotate the camera around the pivot on the Y axis.
    glm::mat4 rotation_y(1.0f);
    rotation_y   = glm::rotate(rotation_y, angle_xy.y, camera.GetRight());
    cam_position = rotation_y * (cam_position - pivot_) + pivot_;
    auto previous_front = camera.GetFront();
    if (!camera.SetFront(glm::normalize(pivot_ - cam_position))) {
        camera.SetPosition(cam_position);   
    }
    return true;
}

bool InputArcball::MousePressed(char button, double dt) {
    mouse_active_ = true;
    return true;
}

bool InputArcball::MouseReleased(char button, double dt) {
    mouse_active_ = false;
    return true;
}

bool InputArcball::WheelMoved(float relative, double dt) {
    auto& camera = device_.GetLevel().GetDefaultCamera();
    float new_fov = camera.GetFovDegrees() + relative * zoom_multiplication_;
    if (new_fov < 1.0f) new_fov = 1.0f;
    if (new_fov > 90.0f) new_fov = 90.0f;
    camera.SetFovDegrees(new_fov);
    return true;
}

void InputArcball::NextFrame() {}

}  // End namespace frame::gui.
