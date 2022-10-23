#pragma once

#include <memory>

#include "frame/device_interface.h"
#include "frame/input_interface.h"

namespace frame::gui {

std::unique_ptr<InputInterface> CreateInputWasd(DeviceInterface* device, float move_multiplication,
                                                float rotation_multiplication);

} // End namespace frame::gui.
