#pragma once

#include <string>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/logger.h"

namespace frame::gui
{

class WindowLogger : public GuiWindowInterface
{
  public:
    WindowLogger(const std::string& name);
    virtual ~WindowLogger() = default;

  public:
    //! @brief Draw callback setting.
    bool DrawCallback() override;
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
    /**
     * @brief Check if this is the end of the software.
     * @return True if this is the end false if not.
     */
    bool End() const override;

  protected:
    //! @brief Log with color.
    void LogWithColor(const LogMessage& log_message) const;

  private:
    frame::Logger& logger_ = frame::Logger::GetInstance();
    std::string name_;
};

} // namespace frame::gui
