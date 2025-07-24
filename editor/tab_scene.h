#pragma once

#include "frame/entity_id.h"
#include "tab_interface.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImNodeFlow.h>
#include <imgui.h>

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

  private:
    bool initialized_ = false;
    ImFlow::ImNodeFlow node_flow_;
};

} // namespace frame::gui
