#pragma once

#include <glm/glm.hpp>
#include <utility>

#include "frame/name_interface.h"

namespace frame
{

class DeviceInterface;
class MaterialInterface;
class StaticMeshInterface;
class UniformInterface;

/**
 * @class PluginInterface
 * @brief This is the plugin interface that will be used for Stream and
 *        other plugin.
 */
class PluginInterface : public NameInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~PluginInterface() = default;
    /**
     * @brief Initialize with the size of the out buffer.
     * @param size: Size of the out buffer.
     */
    virtual void Startup(glm::uvec2 size) = 0;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     * @return Is the loop continuing?
     */
    virtual bool PollEvent(void *event) = 0;
    /**
     * @brief Called before rendering.
     * @param uniform[in, out]: The uniform data.
     * @param device: The device.
     * @param level: The level.
     * @param static_mesh: The static mesh.
     * @param material: The material associated with the mesh.
     */
    virtual void PreRender(
        UniformInterface &uniform,
        DeviceInterface &device,
        StaticMeshInterface &static_mesh,
        MaterialInterface &material) = 0;
    /**
     * @brief Called to update variables, called after the main render
     *        phase.
     * @param level: The level.
     * @return Is the loop continuing?
     */
    virtual bool Update(DeviceInterface &device, double dt = 0.0) = 0;
    //! @brief Called to cleanup at the end.
    virtual void End() = 0;
};

} // End namespace frame.
