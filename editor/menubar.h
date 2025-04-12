#pragma once

#include <glm/glm.hpp>
#include <string>

#include "frame/api.h"
#include "frame/gui/gui_menu_bar_interface.h"
#include "menubar_view.h"

namespace frame::gui
{

/**
 * @class Menubar
 * @brief The menu bar of the editor.
 */
class Menubar : public GuiMenuBarInterface
{
  public:
    /**
     * @brief Default constructor.
     * @param name: The name of the window.
     */
    Menubar(
		const std::string& name,
        MenubarView& menubar_view,
		DeviceInterface& device);
    //! @brief Virtual destructor.
    virtual ~Menubar() = default;

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
     * @brief Is it the end of the gui?
     * @return True if this is the end false if not.
     */
    bool End() const override;

  protected:
    void MenuFile();
    void MenuEdit();
    void MenuView();

  private:
    std::string name_;
    bool end_ = true;
    bool show_logger_ = false;
    bool show_resolution_ = false;
    MenubarView& menubar_view_;
	DeviceInterface& device_;
};

}
