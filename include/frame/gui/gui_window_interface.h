#pragma once

#include "frame/name_interface.h"

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

} // namespace frame::gui.
