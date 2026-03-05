#pragma once

#include "frame/entity_id.h"
#include "tab_interface.h"

namespace frame::gui
{

class TabScene : public TabInterface
{
  public:
    TabScene() : TabInterface("Scene")
    {
    }
    ~TabScene() override = default;

    void Draw(LevelInterface& level) override;
};

} // namespace frame::gui
