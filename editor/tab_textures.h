#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabTextures : public TabInterface {
  public:
    TabTextures() : TabInterface("Textures") {}
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
