#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabPrograms : public TabInterface {
  public:
    std::string GetName() const override { return "Programs"; }
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
