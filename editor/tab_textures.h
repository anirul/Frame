#pragma once

#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_file_dialog.h"
#include "tab_interface.h"

namespace frame::gui
{

class TabTextures : public TabInterface
{
  public:
    explicit TabTextures(DrawGuiInterface& draw_gui)
        : TabInterface("Textures"), draw_gui_(draw_gui)
    {
    }

    void Draw(LevelInterface& level) override;

  private:
    void AddTextureFromFile(LevelInterface& level, const std::string& file);

  private:
    DrawGuiInterface& draw_gui_;
};

} // namespace frame::gui
