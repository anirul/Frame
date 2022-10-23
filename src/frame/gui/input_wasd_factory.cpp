#include "frame/gui/input_wasd_factory.h"

#include <memory>

#include "frame/gui/input_wasd.h"

namespace frame::gui {

std::unique_ptr<frame::InputInterface> CreateInputWasd(DeviceInterface* device,
                                                       float move_multiplication,
                                                       float rotation_multiplication) {
    return std::make_unique<InputWasd>(device, move_multiplication, rotation_multiplication);
}

}  // End namespace frame::gui.
