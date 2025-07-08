#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabTextures : public TabInterface {
  public:
    std::string GetName() const override { return "Textures"; }
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
