#pragma once

#include <vulkan/vulkan.hpp>

#include "frame/camera.h"
#include "frame/device_interface.h"
#include "frame/logger.h"

namespace frame::vulkan
{

/**
 * @class Device
 * @brief This is the OpenGL implementation of the device interface.
 */
class Device : public DeviceInterface
{
  public:
    /**
     * @brief Constructor will initialize the Vulkan context.
     * @param vk_instance: The vk::Instance passed as a void*.
     * @param size: Window size.
     * @param surface: Surface for the drawing.
     * @param dispatch: Dispatch from vulkan.
     */
    Device(
        void* vk_instance,
        glm::uvec2 size,
        vk::SurfaceKHR& surface);
    //! @brief Destructor this is where the memory is freed.
    virtual ~Device();

  public:
    /**
     * @brief Set the stereo mode (by default this is NONE), interocular
     *        distance and focus point.
     * @param stereo_enum: Set the mode the stereo will use.
     * @param interocular_distance: Distance between the eyes.
     * @param focus_point: Point of focus in the 3D scene (if 0 then they
     *        will look in parallel).
     */
    void SetStereo(
        StereoEnum stereo_enum,
        float interocular_distance,
        glm::vec3 focus_point,
        bool invert_left_right) final;
    /**
     * @brief Clear the Screen.
     * @param color: Take a vec4 and make it into a color [0, 1] the last
     * parameter is alpha.
     */
    void Clear(
        const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const final;
    /**
     * @brief Startup the scene.
     * @param level: Move the level into the scene.
     */
    void Startup(std::unique_ptr<LevelInterface>&& level) final;
    /**
     * @brief Add a plugin interface.
     * @param plugin_interface: The plugin interface to be moved.
     */
    void AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface) final;
    /**
     * @brief Get a list of plugin.
     * @return A list of pointer to plugin.
     */
    std::vector<PluginInterface*> GetPluginPtrs() final;
    /**
     * @brief Get plugin names.
     * @return A list of plugin names.
     */
    std::vector<std::string> GetPluginNames() const final;
    /**
     * @brief Remove a plugin by name.
     * @param name: The name of the plugin to remove.
     */
    void RemovePluginByName(const std::string& name) final;
    /** @brief Cleanup the mess. */
    void Cleanup() final;
    /**
     * @brief Resize the window.
     * @param size: The new size of the window.
     */
    void Resize(glm::uvec2 size) final;
    /**
     * @brief Get the size of the window.
     * @return The size of the window.
     */
    glm::uvec2 GetSize() const final;
    /**
     * @brief Display to the screen.
     * @param dt: Delta time from the beginning of the software in seconds.
     */
    void Display(double dt = 0.0) final;
    /**
     * @brief Make a screen shot to a file.
     * @param file: File name of the screenshot (usually with the *.png)
     *        extension it will be dropped at the path where the software is
     *        run.
     */
    void ScreenShot(const std::string& file) const final;
    /**
     * @brief Create a point buffer from a vector of floats.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    std::unique_ptr<BufferInterface> CreatePointBuffer(
        std::vector<float>&& vector) final;
    /**
     * @brief Create an index buffer from a vector of unsigned integer.
     * @param device: A pointer to a device.
     * @param vector: A vector that is moved into the device and level.
     */
    std::unique_ptr<BufferInterface> CreateIndexBuffer(
        std::vector<std::uint32_t>&& vector) final;
    /**
     * @brief Create a static mesh from a vector of floats.
     * @param vector: A vector that is moved into the device and level.
     * @param point_buffer_size: The size of a point in float.
     */
    std::unique_ptr<StaticMeshInterface> CreateStaticMesh(
        const StaticMeshParameter& static_mesh_parameter) final;

  public:
    /**
     * @brief Get the current level.
     * @return a temporary pointer to the current level being run.
     */
    LevelInterface& GetLevel() final
    {
        return *level_.get();
    }
    /**
     * @brief Get the current context.
     * @return A pointer to the current context (this is used by the
     *         windowing system).
     */
    void* GetDeviceContext() const final
    {
        return vk_instance_;
    }
    /**
     * @brief Get the enum describing the stereo situation.
     * @return Return the enum describing the stereo situation.
     */
    StereoEnum GetStereoEnum() const
    {
        return stereo_enum_;
    }
    /**
     * @brief Get the interocular distance.
     * @return Return the interocular distance.
     */
    float GetInteroccularDistance() const
    {
        return interocular_distance_;
    }
    /**
     * @brief Get the focus point.
     * @return Return the focus point.
     */
    glm::vec3 GetFocusPoint() const
    {
        return focus_point_;
    }
    /**
     * @brief Get the application programming interface of the device.
     * @return Return the application programming interface used by the
     *         device.
     */
    RenderingAPIEnum GetDeviceEnum() const final
    {
        return RenderingAPIEnum::VULKAN;
    }

  private:
    // Map of current stored level.
    std::unique_ptr<LevelInterface> level_ = nullptr;
    // Storage of the plugin.
    std::vector<std::unique_ptr<PluginInterface>> plugin_interfaces_ = {};
    // Vulkan stuff.
    vk::Instance vk_instance_ = {};
    vk::PhysicalDevice vk_physical_device_ = {};
    float queue_family_priority_ = 1.0f;
    vk::UniqueDevice vk_unique_device_;
    vk::SurfaceKHR& vk_surface_;
    // Size.
    glm::uvec2 size_ = {0, 0};
    const proto::PixelElementSize pixel_element_size_ =
        json::PixelElementSize_HALF();
    // Rendering pipeline.
    // std::unique_ptr<Renderer> renderer_ = nullptr;
    // Stereo mode.
    StereoEnum stereo_enum_ = StereoEnum::NONE;
    float interocular_distance_ = 0.0f;
    glm::vec3 focus_point_ = glm::vec3(0.0f);
    bool invert_left_right_ = false;
    // Logger for the device.
    const Logger& logger_ = Logger::GetInstance();
};

} // End namespace frame::vulkan.
