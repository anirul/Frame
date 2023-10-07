#pragma once

#include <functional>

#include "frame/name_interface.h"
#include "frame/plugin_interface.h"

namespace frame::gui
{

/**
 * @class GuiWindowInterface
 * @brief Draw GUI Window interface.
 *
 * It has a name interface that will be used as a pointer to the window.
 */
struct GuiWindowInterface : public NameInterface
{
    //! @brief Draw callback setting.
    virtual bool DrawCallback() = 0;
    //! @brief Is it the end of the gui?
    virtual bool End() const = 0;
};

/**
 * @class DrawGuiInterface
 * @brief Interface for drawing GUI elements.
 */
class DrawGuiInterface : public PluginInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~DrawGuiInterface() = default;
    /**
     * @brief Add sub window to the main window.
     * @param callback: A window callback that can add buttons, etc.
     */
    virtual void AddWindow(std::unique_ptr<GuiWindowInterface>&& callback) = 0;
    /**
     * @brief Get a specific window (associated with a name).
     * @param name: The name of the window.
     * @return A pointer to the window.
     */
    virtual GuiWindowInterface& GetWindow(const std::string& name) = 0;
    /**
     * @brief Get all sub window name (title).
     * @return A list of all the sub windows.
     */
    virtual std::vector<std::string> GetWindowTitles() const = 0;
    /**
     * @brief Delete a sub window.
     * @param name: the name of the window to be deleted.
     */
    virtual void DeleteWindow(const std::string& name) = 0;
    /**
     * @brief Is the draw gui active?
     * @param enable: True if enable.
     */
    virtual void SetVisible(bool enable) = 0;
    /**
     * @brief Is the draw gui active?
     * @return True if enable.
     */
    virtual bool IsVisible() const = 0;
};

} // End namespace frame::gui.
