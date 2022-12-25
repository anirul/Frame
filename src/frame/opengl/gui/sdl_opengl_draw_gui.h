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
     * @brief Get name.
     * @return Name.
     */
    std::string GetName() const override { return name_; }
    /**
     * @brief Set name.
     * @return Name.
     */
    void SetName(const std::string& name) override { name_ = name; }
    /**
     * @brief Called before rendering.
     * @param uniform[in, out]: The uniform data.
     * @param device: The device.
     * @param level: The level.
     * @param static_mesh: The static mesh.
     * @param material: The material associated with the mesh.
     */
    void PreRender(UniformInterface& uniform, DeviceInterface* device,
                   StaticMeshInterface* static_mesh, MaterialInterface* material) override {}
    //! @brief Called to cleanup at the end.
    void End() override {}
    /**
     * @brief Is the draw gui active?
     * @param enable: True if enable.
     */
    void SetVisible(bool enable) override { is_visible_ = enable; }
    /**
     * @brief Is the draw gui active?
     * @return True if enable.
     */
    bool IsVisible() const override { return is_visible_; }

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
    void Startup(glm::uvec2 size) override;
    /**
     * @brief Called to update variables, called after the main render phase.
     * @param level: The level.
     * @return Is it still looping or not?
     */
    bool Update(DeviceInterface* device, double dt = 0.0) override;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     */
    bool PollEvent(void* event) override;

   protected:
    std::map<std::string, std::unique_ptr<frame::gui::GuiWindowInterface>> callbacks_ = {};
    WindowInterface* window_interface_                                                = nullptr;
    DeviceInterface* device_interface_                                                = nullptr;
    std::string name_;
    glm::uvec2 size_         = { 0, 0 };
    bool is_keyboard_passed_ = false;
    bool is_visible_         = true;
};

}  // End namespace frame::opengl::gui.
