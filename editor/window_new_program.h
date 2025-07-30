#pragma once

#include "frame/gui/gui_window_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/level_interface.h"

#include <functional>
#include <string>

namespace frame::gui {

class WindowNewProgram : public GuiWindowInterface {
  public:
    WindowNewProgram(
        DrawGuiInterface& draw_gui,
        LevelInterface& level,
        std::function<void()> update_json_callback);
    ~WindowNewProgram() override = default;

    bool DrawCallback() override;
    bool End() const override { return end_; }
    std::string GetName() const override { return name_; }
    void SetName(const std::string& name) override { name_ = name; }

  private:
    DrawGuiInterface& draw_gui_;
    LevelInterface& level_;
    std::function<void()> update_json_callback_;
    char name_buffer_[64] = "";
    char shader_buffer_[64] = "";
    bool end_ = false;
    std::string name_ = "New Program";
};

} // namespace frame::gui
