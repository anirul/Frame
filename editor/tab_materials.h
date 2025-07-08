#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabMaterials : public TabInterface {
  public:
    TabMaterials() : TabInterface("Materials") {}
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
