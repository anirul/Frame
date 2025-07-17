#pragma once

#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_file_dialog.h"
#include <functional>
#include "tab_interface.h"

namespace frame::gui
{

class TabTextures : public TabInterface
{
  public:
    TabTextures(
        DrawGuiInterface& draw_gui,
        std::function<void()> update_json_callback)
        : TabInterface("Textures"),
          draw_gui_(draw_gui),
          update_json_callback_(std::move(update_json_callback))
    {
    }

    void Draw(LevelInterface& level) override;

  private:
    void AddTextureFromFile(LevelInterface& level, const std::string& file);

  private:
    DrawGuiInterface& draw_gui_;
    std::function<void()> update_json_callback_;
};

} // namespace frame::gui
