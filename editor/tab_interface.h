#pragma once

#include "frame/level_interface.h"
#include "frame/name_interface.h"

namespace frame::gui {

class TabInterface : public NameInterface {
  public:
    ~TabInterface() override = default;
    virtual void Draw(LevelInterface& level) = 0;
};

} // namespace frame::gui
