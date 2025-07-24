#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_json_file.h"

#include "tab_materials.h"
#include "tab_programs.h"
#include "tab_scene.h"
#include "tab_textures.h"

namespace frame::gui
{

class WindowLevel : public WindowJsonFile
{
  public:
    WindowLevel(
        DeviceInterface& device,
        DrawGuiInterface& draw_gui,
        const std::string& file_name);
    ~WindowLevel() override = default;

    /** @brief Update the JSON editor with the current level state. */
    void UpdateJsonEditor();

    bool DrawCallback() override;

  private:
    DeviceInterface& device_;
    DrawGuiInterface& draw_gui_;
    TabTextures tab_textures_;
    TabPrograms tab_programs_;
    TabMaterials tab_materials_;
    TabScene tab_scene_;
    bool show_json_ = false;
};

} // namespace frame::gui
