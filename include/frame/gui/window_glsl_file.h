#pragma once

#include "TextEditor.h"
#include <string>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/gui/gui_window_interface.h"
#include "frame/opengl/shader.h"

namespace frame::gui
{

class WindowGlslFile : public GuiWindowInterface
{
  public:
    WindowGlslFile(
        const std::string& file_name,
        DeviceInterface& device,
        const std::string& level_file = "");
    ~WindowGlslFile() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

    /**
     * @brief Compile the current editor text without writing it to disk.
     * @return True on successful compilation, false otherwise.
     */
    bool Compile();
    /**
     * @brief Apply the current editor text by compiling it and, on success,
     * saving the file and reloading the device.
     * @return True on success, false if compilation failed or writing the file
     * failed.
     */
    bool Apply();

    /** @brief Set the text in the internal editor. */
    void SetEditorText(const std::string& text);
    /** @brief Get the text from the internal editor. */
    std::string GetEditorText() const;

  private:
    std::string name_;
    std::string file_name_;
    DeviceInterface& device_;
    TextEditor editor_;
    bool end_ = false;
    std::string error_message_;
    std::string level_file_;
};

} // End namespace frame::gui.
