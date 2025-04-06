#pragma once

#include <glm/glm.hpp>
#include <string>
#include <map>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_resolution.h"

namespace frame::gui
{

/**
 * @class View Windows
 * @brief A container for the view windows.
 */
class ViewWindows
{
  public:
    /**
     * @brief Default constructor.
     * @param name: The name of the window.
     */
    ViewWindows(
		DrawGuiInterface* draw_gui,
		glm::uvec2 size,
		glm::uvec2 desktop_size,
		glm::uvec2 pixel_per_inch);
    //! @brief Virtual destructor.
    virtual ~ViewWindows() = default;

  public:
    void ShowLoggerWindow();
    void ShowResolutionWindow();
    void ShowTexturesWindow(DeviceInterface& device);

  public:
    const WindowResolution* GetWindowResolution() const
    {
        return ptr_window_resolution_;
    }

  protected:
    void CreateLogger(const std::string& name);
    void DeleteLogger(const std::string& name);
    void CreateResolution(const std::string& name);
    void DeleteResolution(const std::string& name);
    void CreateTexture(const std::string& name);
    void DeleteTexture(const std::string& name);

  private:
    std::map<std::string, bool> window_state_;
    DrawGuiInterface* draw_gui_;
    glm::uvec2 size_;
    glm::uvec2 desktop_size_;
    glm::uvec2 pixel_per_inch_;
    WindowResolution* ptr_window_resolution_ = nullptr;
};

} // End namespace frame::gui.
