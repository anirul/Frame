#pragma once

#include "frame/entity_id.h"
#include "tab_interface.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImNodeFlow.h>
#include <imgui.h>
#include <memory>
#include <unordered_map>

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
    void BuildScene(LevelInterface& level);

  private:
    bool initialized_ = false;
    ImFlow::ImNodeFlow node_flow_;
    std::unordered_map<frame::EntityId, std::shared_ptr<ImFlow::BaseNode>>
        nodes_;
};

} // namespace frame::gui
