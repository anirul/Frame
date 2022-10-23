#pragma once

#include <memory>
#include <string>
#include <utility>

#include "frame/api.h"
#include "frame/device_interface.h"
#include "frame/draw_interface.h"
#include "frame/input_interface.h"

namespace frame {

/**
 * @class WindowInterface
 * @brief Interface to a window this is specific to a platform (see Windows, SDL, Linux, OSX, iOS,
 * etc...).
 */
struct WindowInterface {
    //! Virtual destructor.
    virtual ~WindowInterface() = default;
    //! @brief Run the windows interface this will take the current thread.
    virtual void Run() = 0;
    /**
     * @brief Add a drawing interface.
     * @param draw_interface: Move a draw interface to the window object.
     * @return The index of the draw interface.
     */
    virtual int AddDrawInterface(std::unique_ptr<DrawInterface>&& draw_interface) = 0;
    /**
     * @brief Remove a drawing interface.
     * @param index: The index of the draw interface to remove.
     */
    virtual void RemoveDrawInterface(int index) = 0;
    /**
     * @brief Set the input interface (see above).
     * @param input_interface: Move a input interface to the window object.
     */
    virtual void SetInputInterface(std::unique_ptr<InputInterface>&& input_interface) = 0;
    /**
     * @brief Set the unique device (this is suppose to be variable to the one you are using see :
     * DirectX, OpenGL, etc...).
     * @param device: Move a device to the window object.
     */
    virtual void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) = 0;
    /**
     * @brief Get the current device the one that was assign to this window.
     * @return A pointer to a device interface.
     */
    virtual DeviceInterface* GetUniqueDevice() = 0;
    /**
     * @brief Get the size of the window (useful to make a buffer).
     * @return A size {x, y} of the window.
     */
    virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
    /**
     * @brief Get the desktop size.
     * @return A size {x, y} of the desktop.
     */
    virtual std::pair<std::uint32_t, std::uint32_t> GetDesktopSize() const = 0;
    /**
     * @brief Return the context to the window (this is a void* as this can be a Windows HWND, a
     * Linux window or ?).
     * @return A pointer to an underlying window context.
     */
    virtual void* GetWindowContext() const = 0;
    /**
     * @brief Return an graphic context.
     * @return A pointer to an graphic context.
     */
    virtual void* GetGraphicContext() const = 0;
    /**
     * @brief Set the window title (the name of the window).
     * @param title: Window title.
     */
    virtual void SetWindowTitle(const std::string& title) const = 0;
    /**
     * @brief Get the window enum (know if this is an SDL a NONE or other window).
     * @return A window enum.
     */
    virtual WindowEnum GetWindowEnum() const = 0;
    /**
     * @brief Resize the window.
     * @param size: The new window size.
     */
    virtual void Resize(std::pair<std::uint32_t, std::uint32_t> size) = 0;
    /**
     * @brief Set window full screen mode.
     * @param fullscreen_enum: Is it full screen or not?
     */
    virtual void SetFullScreen(FullScreenEnum fullscreen_enum) = 0;
    /**
     * @brief Get window full screen enum.
     * @return The full screen enum.
     */
    virtual FullScreenEnum GetFullScreenEnum() const = 0;
};

}  // End namespace frame.
