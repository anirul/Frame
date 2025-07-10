#pragma once

#include "tab_interface.h"

namespace frame::gui {

class TabTree : public TabInterface {
  public:
    TabTree() : TabInterface("Scene Tree") {}
    void Draw(LevelInterface& level) override;

  private:
    void DrawNode(LevelInterface& level, EntityId id);
};

} // namespace frame::gui
