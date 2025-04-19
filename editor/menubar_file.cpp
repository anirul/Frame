#include "menubar_file.h"

namespace frame::gui
{

MenubarFile::MenubarFile(
    DeviceInterface& device,
    DrawGuiInterface& draw_gui,
    const std::string& file_name)
    : device_(device),
      draw_gui_(draw_gui),
      file_name_(file_name) {}

bool MenubarFile::HasChanged()
{
    if (changed_)
    {
        changed_ = false;
        return true;
    }
    return false;
}

std::string MenubarFile::GetFileName() const
{
    return file_name_;
}

void MenubarFile::SetFleName(const std::string& file_name)
{
    file_name_ = file_name_;
}

FileDialogEnum MenubarFile::GetFileDialogEnum() const
{
    return file_dialog_enum_;
}

void MenubarFile::ShowNewProject()
{
    draw_gui_.AddModalWindow(
        std::make_unique<WindowFileDialog>(
            "json",
            FileDialogEnum::NEW,
            [this](const std::string& file_name)
            {
                file_name_ = file_name;
                file_dialog_enum_ = FileDialogEnum::NEW;
                changed_ = true;
            }));
}

void MenubarFile::ShowOpenProject()
{
    draw_gui_.AddModalWindow(
        std::make_unique<WindowFileDialog>(
            "json",
            FileDialogEnum::OPEN,
            [this](const std::string& file_name)
            {
                file_name_ = file_name;
                file_dialog_enum_ = FileDialogEnum::OPEN;
                changed_ = true;
            }));
}

void MenubarFile::ShowSaveAsProject()
{
    draw_gui_.AddModalWindow(
        std::make_unique<WindowFileDialog>(
            "json",
            FileDialogEnum::SAVE_AS,
            [this](const std::string& file_name)
            {
                file_name_ = file_name;
                file_dialog_enum_ = FileDialogEnum::SAVE_AS;
                changed_ = true;
            }));
}

} // namespace frame::gui
