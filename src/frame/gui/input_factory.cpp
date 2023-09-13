#include "frame/gui/input_factory.h"

#include <memory>

#include "frame/gui/input_arcball.h"
#include "frame/gui/input_wasd.h"
#include "frame/gui/input_wasd_mouse.h"

namespace frame::gui
{

std::unique_ptr<frame::InputInterface> CreateInputWasd(
    DeviceInterface &device,
    float move_multiplication,
    float rotation_multiplication)
{
    return std::make_unique<InputWasd>(
        device, move_multiplication, rotation_multiplication);
}

std::unique_ptr<InputInterface> CreateInputWasdMouse(
    DeviceInterface &device,
    float move_multiplication,
    float rotation_multiplication,
    float translation_multiplication,
    float wheel_multiplication)
{
    return std::make_unique<InputWasdMouse>(
        device,
        move_multiplication,
        rotation_multiplication,
        translation_multiplication,
        wheel_multiplication);
}

std::unique_ptr<InputInterface> CreateInputArcball(
    DeviceInterface &device,
    glm::vec3 pivot,
    float move_multiplication,
    float zoom_multiplication)
{
    return std::make_unique<InputArcball>(
        device, pivot, move_multiplication, zoom_multiplication);
}

} // End namespace frame::gui.
