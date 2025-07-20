#pragma once

#include "frame/name_interface.h"
#include <glm/glm.hpp>

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
    //! @brief Optional initial size for the window (0 means default).
    virtual glm::vec2 GetInitialSize() const
    {
        return glm::vec2(0.0f);
    }
};

} // namespace frame::gui.
