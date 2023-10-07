#pragma once

#include <memory>
#include <string>
#include <utility>

#include "frame/api.h"
#include "frame/device_interface.h"
#include "frame/input_interface.h"
#include "frame/plugin_interface.h"

namespace frame
{

/**
 * @class WindowInterface
 * @brief Interface to a window this is specific to a platform.
 *
 * See Windows, SDL, Linux, OSX, iOS, etc...
 */
struct WindowInterface
{
    //! Virtual destructor.
    virtual ~WindowInterface() = default;
    //! @brief Run the windows interface this will take the current thread.
    virtual void Run(std::function<void()> lambda = [] {}) = 0;
    /**
     * @brief Set the input interface (see above).
     * @param input_interface: Move a input interface to the window object.
     */
    virtual void SetInputInterface(
        std::unique_ptr<InputInterface>&& input_interface) = 0;
    /**
     * @brief Add a callback for a key.
     * @param key: The key to add a callback for.
     * @param func: The callback function to be called in case the key is
     *        pressed.
     */
    virtual void AddKeyCallback(
        std::int32_t key, std::function<bool()> func) = 0;
    /**
     * @brief Set the unique device (this is suppose to be variable to the
     *        one you are using see : DirectX, OpenGL, etc...).
     * @param device: Move a device to the window object.
     */
    virtual void SetUniqueDevice(std::unique_ptr<DeviceInterface>&& device) = 0;
    /**
     * @brief Get the current device the one that was assign to this window.
     * @return A pointer to a device interface.
     */
    virtual DeviceInterface& GetDevice() = 0;
    /**
     * @brief Get the size of the window (useful to make a buffer).
     * @return A size {x, y} of the window.
     */
    virtual glm::uvec2 GetSize() const = 0;
    /**
     * @brief Get DPI
     * @param screen: Screen id.
     * @return Both horizontal and vertical DPI if available or exception.
     */
    virtual glm::vec2 GetPixelPerInch(std::uint32_t screen = 0) const = 0;
    /**
     * @brief Get the desktop size.
     * @return A size {x, y} of the desktop.
     */
    virtual glm::uvec2 GetDesktopSize() const = 0;
    /**
     * @brief Return the context to the window (this is a void* as this can
     *        be a Windows HWND, a Linux window or ?).
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
     * @brief Resize the window.
     * @param fullscreen_enum: Is it full screen or not?
     * @param size: The new window size.
     */
    virtual void Resize(
        glm::uvec2 size,
        FullScreenEnum fullscreen_enum = FullScreenEnum::WINDOW) = 0;
    /**
     * @brief Get window full screen enum.
     * @return The full screen enum.
     */
    virtual FullScreenEnum GetFullScreenEnum() const = 0;
    /**
     * @brief Get the drawing target enum (none, window).
     * @return The drawing target enum (none, window).
     */
    virtual DrawingTargetEnum GetDrawingTargetEnum() const = 0;
};

} // End namespace frame.
