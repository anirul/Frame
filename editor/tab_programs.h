#pragma once

#include "tab_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_program.h"
#include <memory>

namespace frame::gui {

class TabPrograms : public TabInterface {
  public:
    explicit TabPrograms(DrawGuiInterface& draw_gui)
        : TabInterface("Programs"), draw_gui_(draw_gui) {}
    void Draw(LevelInterface& level) override;

  private:
    DrawGuiInterface& draw_gui_;
    std::unique_ptr<WindowProgram> current_program_;
};

} // namespace frame::gui
