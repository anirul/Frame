#pragma once

#include <memory>
#include <string>

#include "Frame/LevelInterface.h"

namespace frame {

/**
 * @class DeviceInterface
 * @brief This is the interface for the device, all the function should be implemented if you want
 * to port this to another rendering pipeline.
 */
struct DeviceInterface {
    /**
     * @brief Clear the Screen.
	 * @param color: A glm vec4 containing the color you want to clear to.
     */
    virtual void Clear(const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const = 0;
    /**
     * @brief Startup the scene.
	 * @param level: The level you want to start the scene with.
     */
    virtual void Startup(std::unique_ptr<LevelInterface>&& level) = 0;
    /**
     * @brief Display to the screen.
	 * @param dt: Delta time from the beginning of the software in seconds.
     */
    virtual void Display(double dt = 0.0) = 0;
    //! @brief Cleanup the mess.
    virtual void Cleanup() = 0;
    /**
     * @brief  Get the current level.
	 * @return A pointer to the level.
     */
    virtual LevelInterface* GetLevel() = 0;
    /**
     * @brief Get a device context on the underlying graphic API.
	 * @return A device context on the underlying graphic API.
     */
    virtual void* GetDeviceContext() const = 0;
    /**
     * @brief Get the underlying API used by this device.
	 * @return A string of the underlying API (OpenGL/Vulkan/???).
     */
    virtual const std::string GetTypeString() const = 0;
    /**
     * @brief Make a screenshot of current frame.
	 * @param file: File name to write the screenshot to (*.png).
     */
    virtual void ScreenShot(const std::string& file) const = 0;
};

}  // End namespace frame.
