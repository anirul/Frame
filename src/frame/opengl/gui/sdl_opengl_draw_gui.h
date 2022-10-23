#pragma once

#include <memory>

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/window_interface.h"

namespace frame::opengl::gui {

/**
 * @class SDL2OpenGLDrawGui
 * @brief Draw GUI elements using SDL2 and OpenGL.
 */
class SDL2OpenGLDrawGui : public frame::gui::DrawGuiInterface {
   public:
    /**
     * @brief Constructor.
     * @param device: The device to use.
     * @param window: The window to use.
     */
    SDL2OpenGLDrawGui(frame::DeviceInterface* device, frame::WindowInterface* window);
    //! @brief Destructor.
    virtual ~SDL2OpenGLDrawGui();

   public:
    /**
     * @brief Add sub window to the main window.
     * @param callback: A window callback that can add buttons, etc.
     */
    void AddWindow(std::unique_ptr<frame::gui::GuiWindowInterface>&& callback) override;
    /**
     * @brief Get all sub window name (title).
     * @return A list of all the sub windows.
     */
    std::vector<std::string> GetWindowTitles() const override;
    /**
     * @brief Delete a sub window.
     * @param name: the name of the window to be deleted.
     */
    void DeleteWindow(const std::string& name) override;
    /**
     * @brief Initialize with the size of the out buffer.
     * @param size: Size of the out buffer.
     */
    void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
    /**
     * @brief This should draw from the device.
     * @param dt: Delta time from the start of the software in seconds.
     */
    bool RunDraw(double dt) override;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     */
    bool PollEvent(void* event) override;

   protected:
    std::map<std::string, std::unique_ptr<frame::gui::GuiWindowInterface>> callbacks_ = {};
    WindowInterface* window_interface_                                                = nullptr;
    DeviceInterface* device_interface_                                                = nullptr;
};

}  // End namespace frame::opengl::gui.
