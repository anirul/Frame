#pragma once

#include <memory>
#include <filesystem>

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/gui_menu_bar_interface.h"
#include "frame/window_interface.h"
#include "frame/uniform_collection_interface.h"

namespace frame::opengl::gui
{

/**
 * @class SDLOpenGLDrawGui
 * @brief Draw GUI elements using SDL and OpenGL.
 */
class SDLOpenGLDrawGui : public frame::gui::DrawGuiInterface
{
  public:
    /**
     * @brief Constructor.
     * @param window: The window to use.
	 * @param font_path: The path to the font.
     */
    SDLOpenGLDrawGui(
		frame::WindowInterface& window,
		const std::filesystem::path& font_path,
		float font_size);
    //! @brief Destructor.
    virtual ~SDLOpenGLDrawGui();

  public:
    /**
     * @brief Get name.
     * @return Name.
     */
    std::string GetName() const override
    {
        return name_;
    }
    /**
     * @brief Set name.
     * @return Name.
     */
    void SetName(const std::string& name) override
    {
        name_ = name;
    }
    /**
     * @brief Called before rendering.
     * @param uniform[in, out]: The uniform data.
     * @param device: The device.
     * @param level: The level.
     * @param static_mesh: The static mesh.
     * @param material: The material associated with the mesh.
     */
    void PreRender(
        UniformCollectionInterface& uniform,
        DeviceInterface& device,
        StaticMeshInterface& static_mesh,
        MaterialInterface& material) override
    {
    }
    //! @brief Called to cleanup at the end.
    void End() override
    {
    }
    /**
     * @brief Is the draw gui active?
     * @param enable: True if enable.
     */
    void SetVisible(bool enable) override
    {
        is_visible_ = enable;
    }
    /**
     * @brief Is the draw gui active?
     * @return True if enable.
     */
    bool IsVisible() const override
    {
        return is_visible_;
    }

  public:
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     * @return True if the event is captured.
     */
    void SetKeyboardPassed(bool is_keyboard_passed) override
	{
		is_keyboard_passed_locked_ = is_keyboard_passed;
	}
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     * @return True if the event is captured.
     */
    bool IsKeyboardPassed() const override
    {
		return is_keyboard_passed_locked_;
    }
    /**
     * @brief Get the device.
     * @return The device.
     */
    DeviceInterface& GetDevice() override
    {
        return device_;
    }

  public:
    /**
     * @brief Add sub window to the main window.
     * @param callback: A window callback that can add buttons, etc.
     */
    void AddWindow(
		std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    /**
     * @brief Add a overlay window.
     * @param position: The position of the window.
     * @param callback: A window callback that can add buttons, etc.
     *
     * Overlay window are drawn on top of the main window and are not
     * affected. Also note that they are only display when the main window
     * is fullscreen.
     */
    void AddOverlayWindow(
        glm::vec2 position,
		glm::vec2 size,
		std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    /**
     * @brief Add a modal window.
     * @param callback: A window callback that can add buttons, etc.
     *
     * If the callback return is 0 the callback stay and if it is other it is
     * removed. This will trigger an internal boolean that will decide if the
     * modal is active or not. Set the modal window. Replaces any current modal
     * and opens it next frame.
     */
    void AddModalWindow(
        std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    /**
     * @brief Get a specific window (associated with a name).
     * @param name: The name of the window.
     * @return A pointer to the window.
     */
    frame::gui::GuiWindowInterface& GetWindow(
		const std::string& name) override;
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
     * @brief Set a menu bar to the main window.
     * @param callback: A menu bar for the main window.
     *
     * If the callback return is 0 the callback stay and if it is other it is
     * removed. This will trigger an internal boolean that will decide if the
     * modal is active or not.
     */
    void SetMenuBar(
		std::unique_ptr<frame::gui::GuiMenuBarInterface> callback) override;
    /**
     * @brief Get the menu bar.
     * @return The menu bar.
     */
    frame::gui::GuiMenuBarInterface& GetMenuBar() override;
    /**
     * @Remove the menu bar.
     */
    void RemoveMenuBar() override;
    /**
     * @brief Initialize with the size of the out buffer.
     * @param size: Size of the out buffer.
     */
    void Startup(glm::uvec2 size) override;
    /**
     * @brief Called to update variables, called after the main render
     *        phase.
     * @param level: The level.
     * @return Is it still looping or not?
     */
    bool Update(DeviceInterface& device, double dt = 0.0) override;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     */
    bool PollEvent(void* event) override;

  protected:
    struct CallbackData {
		std::unique_ptr<frame::gui::GuiWindowInterface> callback = nullptr;
        glm::vec2 position = { 0.0f, 0.0f };
        glm::vec2 size = { 0.0f, 0.0f };
    };
    std::map<std::string, CallbackData> window_callbacks_ = {};
    std::map<std::string, CallbackData> overlay_callbacks_ = {};
	std::unique_ptr<frame::gui::GuiWindowInterface> modal_callback_ = nullptr;
    std::unique_ptr<frame::gui::GuiMenuBarInterface> menubar_callback_ =
		nullptr;
	bool start_modal_ = false;
	std::filesystem::path font_path_;
    WindowInterface& window_;
    DeviceInterface& device_;
    std::string name_;
    glm::uvec2 size_ = {0, 0};
	glm::uvec2 next_window_position_ = {0, 0};
	glm::uvec2 original_image_size_ = {0, 0};
	bool is_keyboard_passed_locked_ = false;
    bool is_keyboard_passed_ = false;
    bool is_visible_ = true;
	float font_size_ = 20.0f;
};

} // End namespace frame::opengl::gui.
