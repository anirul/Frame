#pragma once

#include <glm/glm.hpp>
#include <string>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"

namespace frame::gui
{

/**
 * @class WindowFileDialog
 * @brief File dialog window.
 */
enum class FileDialogEnum
{
	UNKNOWN = 0,
	NEW,
    OPEN,
    SAVE_AS,
};

class WindowFileDialog : public GuiWindowInterface
{
  public:
    /**
     * @brief Default constructor.
     * @param extention: The file extention.
     * @param file_dialog_enum: Type of dialog (new, open, save as).
     */
    WindowFileDialog(
		const std::string& extension,
		FileDialogEnum file_dialog_enum,
		std::function<void(const std::string&)> get_file);
    //! @brief Virtual destructor.
    virtual ~WindowFileDialog() = default;

  public:
    //! @brief Draw callback setting.
    bool DrawCallback() override;
    /**
     * @brief Check if this is the end of the software.
     * @return True if this is the end false if not.
     */
    bool End() const override;
    /**
     * @brief Get the name of the window.
     * @return The name of the window.
     */
    std::string GetName() const override;
    /**
     * @brief Set the name of the window.
     * @param name: The name of the window.
     */
    void SetName(const std::string& name) override;

  private:
    std::string name_;
    std::string extension_;
    std::string file_name_;
    FileDialogEnum file_dialog_enum_;
    std::function<void(const std::string&)> get_file_;
	bool end_ = false;
};

} // End namespace frame::gui.
