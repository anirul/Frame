#pragma once

#include "frame/name_interface.h"

namespace frame::gui
{

/**
 * @class GuiMenuBar
 * @brief Draw GUI menu bar interface.
 */
class GuiMenuBarInterface : public NameInterface
{
	//! @brief Draw callback setting.
    virtual bool DrawCallback() = 0;
	//! @brief Is it the end of the gui?
    virtual bool End() const = 0;
};

}
