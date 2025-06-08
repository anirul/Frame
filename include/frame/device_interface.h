#pragma once

#include <memory>
#include <string>

#include "frame/api.h"
#include "frame/buffer_interface.h"
#include "frame/level_interface.h"
#include "frame/plugin_interface.h"
#include "frame/texture_interface.h"

namespace frame
{

/**
 * @class DeviceInterface
 * @brief This is the interface for the device, all the function should be
 *        implemented if you want to port this to another rendering
 *        pipeline.
 */
class DeviceInterface
{
  public:
    /**
     * @brief Clear the Screen.
     * @param color: A vec4 containing the color you want to clear to.
     */
    virtual void Clear(
        const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const = 0;
    /**
     * @brief Startup the scene.
     * @param level: The level you want to start the scene with.
     */
    virtual void Startup(std::unique_ptr<LevelInterface>&& level) = 0;
    /**
     * @brief Add a drawing interface.
     * @param draw_interface: Move a draw interface to the window object.
     */
    virtual void AddPlugin(
        std::unique_ptr<PluginInterface>&& plugin_interface) = 0;
    /**
     * @brief Get a list of plugin.
     * @return A list of pointer to plugin.
     */
    virtual std::vector<PluginInterface*> GetPluginPtrs() = 0;
    /**
     * @brief Get name and id of plugin.
     * @return A map containing names and id of plugin.
     */
    virtual std::vector<std::string> GetPluginNames() const = 0;
    /**
     * @brief Remove a drawing interface.
     * @param index: The index of the draw interface to remove.
     */
    virtual void RemovePluginByName(const std::string& name) = 0;
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
    virtual void Resize(glm::uvec2 size) = 0;
    /**
     * @brief Get the size of the window.
     * @return The size of the window.
     */
    virtual glm::uvec2 GetSize() const = 0;
    /**
     * @brief  Get the current level.
     * @return A pointer to the level.
     */
    virtual LevelInterface& GetLevel() = 0;
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
     * @brief Set the stereo mode (by default this is NONE), interocular
     *        distance and focus point.
     * @param stereo_enum: Set the mode the stereo will use.
     * @param interocular_distance: Distance between the eyes.
     * @param focus_point: Point of focus in the 3D scene (if 0 then they
     *        will look in parallel).
     * @param invert_left_right: Invert the left and right view.
     */
    virtual void SetStereo(
        StereoEnum stereo_enum,
        float interocular_distance,
        glm::vec3 focus_point = glm::vec3(0.0f),
        bool invert_left_right = false) = 0;
    /**
     * @brief Get the application programming interface of the device.
     * @return Return the application programming interface used by the
     *         device.
     */
    virtual RenderingAPIEnum GetDeviceEnum() const = 0;
    /**
     * @brief Get the enum describing the stereo situation.
     * @return Return the enum describing the stereo situation.
     */
    virtual StereoEnum GetStereoEnum() const = 0;
    /**
     * @brief Get the interocular distance.
     * @return Return the interocular distance.
     */
    virtual float GetInteroccularDistance() const = 0;
    /**
     * @brief Get the focus point.
     * @return Return the focus point.
     */
    virtual glm::vec3 GetFocusPoint() const = 0;
    /**
     * @brief Create a point buffer from a vector of floats.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    virtual std::unique_ptr<BufferInterface> CreatePointBuffer(
        std::vector<float>&& vector) = 0;
    /**
     * @brief Create an index buffer from a vector of unsigned integer.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    virtual std::unique_ptr<BufferInterface> CreateIndexBuffer(
        std::vector<std::uint32_t>&& vector) = 0;
    /**
     * @brief Create a static mesh from a vector of floats and a vector of
     *        float.
     * @param static_mesh_parameter: A struct containing the parameters to
     *        create a static mesh.
     * @return A unique pointer to a static mesh.
     */
    virtual std::unique_ptr<StaticMeshInterface> CreateStaticMesh(
        const StaticMeshParameter& static_mesh_parameter) = 0;
};

} // End namespace frame.
