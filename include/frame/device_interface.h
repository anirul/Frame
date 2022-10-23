#pragma once

#include <memory>
#include <string>

#include "frame/api.h"
#include "frame/buffer_interface.h"
#include "frame/level_interface.h"

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
     * @brief Resize the window.
     * @param size: The new size of the window.
     */
    virtual void Resize(std::pair<std::uint32_t, std::uint32_t> size) = 0;
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
     * @brief Make a screenshot of current frame.
     * @param file: File name to write the screenshot to (*.png).
     */
    virtual void ScreenShot(const std::string& file) const = 0;
    /**
     * @brief Get the application programming interface of the device.
     * @return Return the application programming interface used by the device.
     */
    virtual DeviceEnum GetDeviceEnum() const = 0;
    /**
     * @brief Create a point buffer from a vector of floats.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    virtual std::unique_ptr<BufferInterface> CreatePointBuffer(std::vector<float>&& vector) = 0;
    /**
     * @brief Create an index buffer from a vector of unsigned integer.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    virtual std::unique_ptr<BufferInterface> CreateIndexBuffer(
        std::vector<std::uint32_t>&& vector) = 0;
    /**
     * @brief Create a static mesh from a vector of floats and a vector of float.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    virtual std::unique_ptr<StaticMeshInterface> CreateStaticMesh(
        std::vector<float>&& vector, std::uint32_t point_buffer_size = 3) = 0;
    /**
     * @brief Create a 2d texture from a structure.
     * @param parameters: Parameters for the creation of the texture.
     * @return A unique pointer to a 2d texture.
     */
    virtual std::unique_ptr<TextureInterface> CreateTexture(
        const TextureParameter& texture_parameter) = 0;
};

}  // End namespace frame.
