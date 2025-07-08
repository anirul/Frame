#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabMaterials : public TabInterface {
  public:
    std::string GetName() const override { return "Materials"; }
    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
