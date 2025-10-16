#include "frame/gui/input_wasd_mouse.h"

namespace frame::gui
{

bool InputWasdMouse::KeyPressed(char key, double dt)
{
    if (key == KEY_LSHIFT || key == KEY_RSHIFT)
        shift_ = true;
    const float inc = move_multiplication_ * static_cast<float>(dt);
    auto& camera = device_.GetLevel().GetDefaultCamera();
    auto position = camera.GetPosition();
    auto right = camera.GetRight();
    auto up = camera.GetUp();
    auto front = camera.GetFront();
    if (key == 'a')
    {
        position -= right * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'd')
    {
        position += right * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'e')
    {
        position += up * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'q')
    {
        position -= up * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 'w')
    {
        position += front * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    if (key == 's')
    {
        position -= front * inc * ((shift_) ? move_shift_ : 1.0f);
    }
    camera.SetPosition(position);
    return true;
}

bool InputWasdMouse::KeyReleased(char key, double dt)
{
    if (key == KEY_LSHIFT || key == KEY_RSHIFT)
        shift_ = false;
    return true;
}

bool InputWasdMouse::MouseMoved(
    glm::vec2 position, glm::vec2 relative, double dt)
{
    if (!mouse_button_pressed_)
        return true;

    auto& camera = device_.GetLevel().GetDefaultCamera();
    auto front = camera.GetFront();
    auto right = camera.GetRight();
    auto up = camera.GetUp();

    // Left button = rotation.
    if (mouse_button_pressed_ == 19)
    {
        const float inc = rotation_multiplication_ * static_cast<float>(dt);
        if (relative.x)
        {
            front += right * relative.x * inc;
        }
        if (relative.y)
        {
            front -= up * relative.y * inc;
        }
        camera.SetFront(front);
        camera.SetUp({0, 1, 0});
    }
    // Right button = translation.
    else if (mouse_button_pressed_ == 23)
    {
        const float inc = translation_multiplication_ * static_cast<float>(dt);
        auto position = camera.GetPosition();
        if (relative.x)
        {
            position -= right * relative.x * inc;
        }
        if (relative.y)
        {
            position += up * relative.y * inc;
        }
        camera.SetPosition(position);
    }
    return true;
}

bool InputWasdMouse::MousePressed(char button, double dt)
{
    if (!mouse_button_pressed_)
    {
        mouse_button_pressed_ = button;
    }
    return true;
}

bool InputWasdMouse::MouseReleased(char button, double dt)
{
    if (mouse_button_pressed_ == button)
    {
        mouse_button_pressed_ = 0;
    }
    return true;
}

bool InputWasdMouse::WheelMoved(float relative, double dt)
{
    auto& camera = device_.GetLevel().GetDefaultCamera();
    auto position = camera.GetPosition();
    auto front = camera.GetFront();

    position +=
        front * relative * wheel_multiplication_ * static_cast<float>(dt);

    camera.SetPosition(position);

    return true;
}

void InputWasdMouse::NextFrame()
{
}

} // End namespace frame::gui.
