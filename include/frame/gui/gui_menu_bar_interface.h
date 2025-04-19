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
  public:
    //! @brief Virtual destructor.
    virtual ~GuiMenuBarInterface() = default;
    //! @brief Draw callback setting.
    virtual bool DrawCallback() = 0;
    //! @brief Is it the end of the gui?
    virtual bool End() const = 0;
};

}
