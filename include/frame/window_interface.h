#pragma once

#include <memory>
#include <string>
#include <utility>

#include "Frame/DeviceInterface.h"
#include "Frame/DrawInterface.h"
#include "Frame/InputInterface.h"

namespace frame {

/**
 * @class WindowInterface
 * @brief  Interface to a window this is specific to a platform (see Windows, SDL, Linux, OSX, iOS,
 * etc...).
 */
struct WindowInterface {
    //! Virtual destructor.
    virtual ~WindowInterface() = default;
    //! @brief Run the windows interface this will take the current thread.
    virtual void Run() = 0;
    /**
     * @brief Set the drawing interface (see above).
     * @param draw_interface: Move a draw interface to the window object.
     */
    virtual void SetDrawInterface(std::unique_ptr<DrawInterface>&& draw_interface) = 0;
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
     * @brief Return the context to the window (this is a void* as this can be a Windows HWND, a
     * Linux window or ?).
     * @return A pointer to an underlying window context.
     */
    virtual void* GetWindowContext() const = 0;
    /**
     * @brief Set the window title (the name of the window).
	 * @param title: Window title.
     */
    virtual void SetWindowTitle(const std::string& title) const = 0;
};

}  // End namespace frame.
