#pragma once

#include "frame/entity_id.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_file_dialog.h"
#include "tab_interface.h"
#include <functional>
#include <string>

namespace frame::gui
{

class TabTextures : public TabInterface
{
  public:
    TabTextures(
        DrawGuiInterface& draw_gui, std::function<void()> update_json_callback)
        : TabInterface("Textures"), draw_gui_(draw_gui),
          update_json_callback_(std::move(update_json_callback))
    {
    }

    void Draw(LevelInterface& level) override;

  private:
    void AddTextureFromFile(LevelInterface& level, const std::string& file);
    void RemoveSelectedTexture(LevelInterface& level);
    bool IsTextureUsed(const LevelInterface& level, EntityId id) const;
    void CloseTextureWindows(const std::string& name);

  private:
    DrawGuiInterface& draw_gui_;
    std::function<void()> update_json_callback_;
    frame::EntityId selected_texture_id_ = frame::NullId;
};

} // namespace frame::gui
