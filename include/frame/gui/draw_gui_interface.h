#pragma once

#include <functional>

#include "frame/gui/gui_window_interface.h"
#include "frame/gui/gui_menu_bar_interface.h"
#include "frame/name_interface.h"
#include "frame/plugin_interface.h"

namespace frame::gui
{

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
    virtual void AddWindow(std::unique_ptr<GuiWindowInterface> callback) = 0;
    /**
     * @brief Add a overlay window.
     * @param position: The position of the window.
     * @param callback: A window callback that can add buttons, etc.
     *
     * Overlay window are drawn on top of the main window and are not
     * affected. Also note that they are only display when the main window
     * is fullscreen.
     */
    virtual void AddOverlayWindow(
        glm::vec2 position,
        glm::vec2 size,
		std::unique_ptr<GuiWindowInterface> callback) = 0;
    /**
     * @brief Add a modal window.
     * @param callback: A window callback that can add buttons, etc.
     *
     * If the callback return is 0 the callback stay and if it is other it is
     * removed. This will trigger an internal boolean that will decide if the
     * modal is active or not.
     */
    virtual void AddModalWindow(
        std::unique_ptr<GuiWindowInterface> callback) = 0;
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
     * @param name: the name of the window or overlay to be deleted.
     */
    virtual void DeleteWindow(const std::string& name) = 0;
    /**
	 * @brief Set a menu bar to the main window.
	 * @param callback: A menu bar for the main window.
	 * 
     * If the callback return is 0 the callback stay and if it is other it is
     * removed. This will trigger an internal boolean that will decide if the
     * modal is active or not.
     */
    virtual void SetMenuBar(std::unique_ptr<GuiMenuBarInterface> callback) = 0;
    /**
	 * @brief Get the menu bar.
	 * @return The menu bar.
	 */
    virtual GuiMenuBarInterface& GetMenuBar() = 0;
    /**
	 * @Remove the menu bar.
	 */
    virtual void RemoveMenuBar() = 0;
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
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     * @return True if the event is captured.
     */
    virtual void SetKeyboardPassed(bool is_keyboard_passed) = 0;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     * @return True if the event is captured.
     */
    virtual bool IsKeyboardPassed() const = 0;
};

} // End namespace frame::gui.
