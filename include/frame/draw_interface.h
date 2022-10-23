#pragma once

#include <cinttypes>
#include <utility>

namespace frame {

/**
 * @class DrawInterface
 * @brief This is the structure that define what draw has to do this is specific to a drawing
 * interface (see DirectX, OpenGL, Metal, Vulkan, etc...).
 */
struct DrawInterface {
	//! @brief Virtual destructor.
    virtual ~DrawInterface() = default;
    /**
     * @brief Initialize with the size of the out buffer.
	 * @param size: Size of the out buffer.
     */
    virtual void Startup(std::pair<std::uint32_t, std::uint32_t> size) = 0;
    /**
     * @brief This should draw from the device.
	 * @param dt: Delta time from the start of the software in seconds.
     */
    virtual bool RunDraw(double dt) = 0;
    /**
     * @brief Poll event.
     * @param event: The event to be polled.
     */
    virtual bool PollEvent(void* event) = 0;
};

}  // End namespace frame.
