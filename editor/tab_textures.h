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
    using ImportTextureCallback =
        std::function<void(const std::string& file, bool as_cubemap)>;

    TabTextures(
        DrawGuiInterface& draw_gui,
        std::function<void()> update_json_callback,
        ImportTextureCallback import_texture_callback)
        : TabInterface("Textures"), draw_gui_(draw_gui),
          update_json_callback_(std::move(update_json_callback)),
          import_texture_callback_(std::move(import_texture_callback))
    {
    }

    void Draw(LevelInterface& level) override;
    void ResetSelection();

  private:
    void ShowImportTextureDialog(bool as_cubemap);
    void AddTextureFromFile(const std::string& file, bool as_cubemap);
    void RemoveSelectedTexture(LevelInterface& level);
    bool HasTextureId(const LevelInterface& level, EntityId id) const;
    bool IsTextureUsed(const LevelInterface& level, EntityId id) const;
    void CloseTextureWindows(const std::string& name);

  private:
    DrawGuiInterface& draw_gui_;
    std::function<void()> update_json_callback_;
    ImportTextureCallback import_texture_callback_;
    frame::EntityId selected_texture_id_ = frame::NullId;
};

} // namespace frame::gui
