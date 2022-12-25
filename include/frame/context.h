#pragma once

#include "frame/window_interface.h"
#include "frame/device_interface.h"
#include "frame/level_interface.h"

namespace frame {

struct Context {
    WindowInterface* window;
    DeviceInterface* device;
    LevelInterface* level;
};

}  // End namespace frame.
