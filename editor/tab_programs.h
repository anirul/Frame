#pragma once

#include "tab_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include <functional>

namespace frame::gui {

class TabPrograms : public TabInterface {
  public:
    TabPrograms(DrawGuiInterface& draw_gui, std::function<void()> update_json_callback)
        : TabInterface("Programs"), draw_gui_(draw_gui),
          update_json_callback_(std::move(update_json_callback)) {}
    void Draw(LevelInterface& level) override;

  private:
    void RemoveSelectedProgram(LevelInterface& level);
    bool IsProgramUsed(const LevelInterface& level, EntityId id) const;

  private:
    DrawGuiInterface& draw_gui_;
    std::function<void()> update_json_callback_;
    frame::EntityId selected_program_id_ = frame::NullId;
};

} // namespace frame::gui
