#pragma once

#include <glm/glm.hpp>
#include <string>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"

namespace frame::gui
{

class WindowTexture : public GuiWindowInterface
{
  public:
    /**
     * @brief Default constructor.
     * @param name: The name of the window.
     * @param device: The device.
     */
    WindowTexture(TextureInterface& texture_interface);
    //! @brief Virtual destructor.
    virtual ~WindowTexture() = default;

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
     * @brief Get the window size.
     * This will vary according to what full screen mode is selected!
     * @return The size of the window.
     */
    glm::uvec2 GetSize() const;
    /**
     * @brief Check if this is the end of the software.
     * @return True if this is the end false if not.
     */
    bool End() const override;

  private:
    std::string name_;
    glm::uvec2 size_;
    TextureInterface& texture_interface_;
};

} // End namespace frame::gui.
