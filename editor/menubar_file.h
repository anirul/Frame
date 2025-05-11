#pragma once

#include <glm/glm.hpp>
#include <string>
#include <map>

#include "frame/api.h"
#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_file_dialog.h"

namespace frame::gui
{

/**
 * @class File Windows
 * @brief A container for the file windows.
 */
class MenubarFile
{
  public:
    MenubarFile(
		DeviceInterface& device,
        DrawGuiInterface& draw_gui,
		const std::string& file_name);
    virtual ~MenubarFile() = default;

  public:
    bool HasChanged();
    std::string GetFileName() const;
    void SetFleName(const std::string& file_name);
    FileDialogEnum GetFileDialogEnum() const;
    void ShowNewProject();
    void ShowOpenProject();
    void ShowSaveAsProject();
    void TrySaveFile();

  private:
    bool changed_ = false;
    std::string file_name_;
    DeviceInterface& device_;
    DrawGuiInterface& draw_gui_;
    FileDialogEnum file_dialog_enum_ = FileDialogEnum::UNKNOWN;
};

} // namespace frame::gui
