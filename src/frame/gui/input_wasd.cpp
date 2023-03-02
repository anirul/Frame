#include "frame/gui/input_wasd.h"

namespace frame::gui {

bool InputWasd::KeyPressed(char key, double dt) {
    if (key == KEY_LSHIFT || key == KEY_RSHIFT) shift_ = true;
    const float inc = move_multiplication_ * static_cast<float>(dt);
    auto& camera    = device_.GetLevel().GetDefaultCamera();
    auto position   = camera.GetPosition();
    auto right      = camera.GetRight();
    auto up         = camera.GetUp();
    auto front      = camera.GetFront();
    if (key == 'a') {
        position -= right * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'd') {
        position += right * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'w') {
        position += front * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 's') {
        position -= front * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    camera.SetPosition(position);
    return true;
}

bool InputWasd::KeyReleased(char key, double dt) {
    if (key == KEY_LSHIFT || key == KEY_RSHIFT) shift_ = false;
    return true;
}

bool InputWasd::MouseMoved(glm::vec2 position, glm::vec2 relative, double dt) {
    if (!mouse_active_) return true;
    const float inc = rotation_multiplication_ * static_cast<float>(dt);
    auto& camera    = device_.GetLevel().GetDefaultCamera();
    auto front      = camera.GetFront();
    auto right      = camera.GetRight();
    auto up         = camera.GetUp();
    if (relative.x) {
        front += right * relative.x * inc;
    }
    if (relative.y) {
        front -= up * relative.y * inc;
    }
    camera.SetFront(front);
    camera.SetUp({ 0, 1, 0 });
    return true;
}

bool InputWasd::MousePressed(char button, double dt) {
    mouse_active_ = true;
    return true;
}

bool InputWasd::MouseReleased(char button, double dt) {
    mouse_active_ = false;
    return true;
}

bool InputWasd::WheelMoved(float relative, double dt) { return true; }

void InputWasd::NextFrame() {}

}  // End namespace frame::gui.
