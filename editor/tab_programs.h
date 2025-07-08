#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabPrograms : public TabInterface {
  public:
    TabPrograms() : TabInterface("Programs") {}
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
