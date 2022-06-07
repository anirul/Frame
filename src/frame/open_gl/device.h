#pragma once

#include <SDL2/SDL.h>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>

#include "frame/camera.h"
#include "frame/device_interface.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/open_gl/buffer.h"
#include "frame/open_gl/material.h"
#include "frame/open_gl/program.h"
#include "frame/open_gl/renderer.h"
#include "frame/open_gl/static_mesh.h"
#include "frame/open_gl/texture.h"
#include "frame/uniform_interface.h"

namespace frame::opengl {

/**
 * @class Device
 * @brief This is the OpenGL implementation of the device interface.
 */
class Device : public DeviceInterface {
   public:
    //! @brief Constructor will initialize the GL context and make the GLEW init.
    Device(void* gl_context, const std::pair<std::uint32_t, std::uint32_t> size);
    //! @brief Destructor this is where the memory is freed.
    virtual ~Device();

   public:
    /**
     * @brief Clear the Screen.
     * @param color: Take a vec4 and make it into a color [0, 1] the last parameter is alpha.
     */
    void Clear(const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const final;
    /**
     * @brief Startup the scene.
     * @param level: Move the level into the scene.
     */
    void Startup(std::unique_ptr<LevelInterface>&& level) final;
    /** @brief Cleanup the mess. */
    void Cleanup() final;
    /**
     * @brief Display to the screen.
     * @param dt: Delta time from the beginning of the software in seconds.
     */
    void Display(double dt = 0.0) final;
    /**
     * @brief Make a screen shot to a file.
     * @param file: File name of the screenshot (usually with the *.png) extension it will be
     * dropped at the path where the software is run.
     */
    void ScreenShot(const std::string& file) const final;

   public:
    /**
     * @brief Get the current level.
     * @return a temporary pointer to the current level being run.
     */
    LevelInterface* GetLevel() final { return level_.get(); }
    /**
     * @brief Get the current context.
     * @return A pointer to the current context (this is used by the windowing system).
     */
    void* GetDeviceContext() const final { return gl_context_; }
    /**
     * @brief Get type string.
     * @return return the system being run (this will return "OpenGL").
     */
    const std::string GetTypeString() const final { return "OpenGL"; }

   private:
    // Map of current stored level.
    std::unique_ptr<LevelInterface> level_ = nullptr;
    // Open GL context.
    void* gl_context_ = nullptr;
    // Constants.
    const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
    const proto::PixelElementSize pixel_element_size_   = proto::PixelElementSize_HALF();
    // Rendering pipeline.
    std::unique_ptr<Renderer> renderer_ = nullptr;
    const Logger& logger_               = Logger::GetInstance();
};

}  // End namespace frame::opengl.
