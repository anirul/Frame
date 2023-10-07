#pragma once

#include <memory>

#include "frame/logger.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/scoped_bind.h"
#include "frame/texture_interface.h"

namespace frame::opengl
{

enum class FrameTextureType : std::int32_t
{
    TEXTURE_2D = -1,
    CUBE_MAP_POSITIVE_X = 0,
    CUBE_MAP_NEGATIVE_X = 1,
    CUBE_MAP_POSITIVE_Y = 2,
    CUBE_MAP_NEGATIVE_Y = 3,
    CUBE_MAP_POSITIVE_Z = 4,
    CUBE_MAP_NEGATIVE_Z = 5,
};

enum class FrameColorAttachment : std::int32_t
{
    COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0,
    COLOR_ATTACHMENT1 = GL_COLOR_ATTACHMENT1,
    COLOR_ATTACHMENT2 = GL_COLOR_ATTACHMENT2,
    COLOR_ATTACHMENT3 = GL_COLOR_ATTACHMENT3,
    COLOR_ATTACHMENT4 = GL_COLOR_ATTACHMENT4,
    COLOR_ATTACHMENT5 = GL_COLOR_ATTACHMENT5,
    COLOR_ATTACHMENT6 = GL_COLOR_ATTACHMENT6,
    COLOR_ATTACHMENT7 = GL_COLOR_ATTACHMENT7,
};

/**
 * @class FrameBuffer
 * @brief Frame buffer class is a packer around the frame buffer concept
 * from OpenGL. This will be derived from the bind interface as the frame
 * buffer can be bound to the current context.
 */
class FrameBuffer : public BindInterface
{
  public:
    //! @brief Constructor this will create a default frame buffer.
    FrameBuffer();
    //! @brief Destructor this will destroy in the RAII way.
    virtual ~FrameBuffer();

  public:
    /**
     * @brief From the bind interface this will bind the framebuffer to the
     * current context the slot parameter can be forgotten as it has no sens
     * with frame buffers.
     * @param slot: Should be ignored.
     */
    void Bind(const unsigned int slot = 0) const override;
    //! @brief From the bind interface this will unbind the current frame
    //! buffer from the context.
    void UnBind() const override;
    /**
     * @brief Attach a render buffer to the current frame buffer, warning
     * this will bind the frame buffer & the render buffer in case it is not
     * yet!
     * @param render: The render buffer that will be attach to the frame
     * buffer.
     */
    void AttachRender(const RenderBuffer& render) const;
    /**
     * @brief Attach a texture to the frame buffer, warning this will bind
     * the frame buffer and the texture in case it is not yet!
     * @param texture_id: Id of the texture.
     * @param frame_color_attachment: On which frame do you need the
     * attachment to be made.
     * @param frame_texture_type: What kind of texture is it normal or
     * cubemap element.
     * @param mipmap: Mipmap level.
     */
    void AttachTexture(
        unsigned int texture_id,
        const FrameColorAttachment frame_color_attachment =
            FrameColorAttachment::COLOR_ATTACHMENT0,
        const FrameTextureType frame_texture_type =
            FrameTextureType::TEXTURE_2D,
        const int mipmap = 0) const;
    /**
     * @brief Define an array of buffers into which outputs from the
     * fragment shader data will be written.
     * @param size: Which draw buffer should be drawn upon [1, 8].
     */
    void DrawBuffers(const std::uint32_t size = 1);
    /**
     * @brief For debug return a string of a potential error.
     * @return String that contain the error (if any).
     */
    const std::string GetStatus() const;

  public:
    /**
     * @brief Convert from OpenGL to FrameColorAttachment.
     * @param i: OpenGL frame color attachment.
     * @return Frame color attachment in local system.
     */
    static FrameColorAttachment GetFrameColorAttachment(const int i);
    /**
     * @brief Convert from OpenGL to frame texture type.
     * @param i: OpenGL frame texture type.
     * @return Frame texture type in local system.
     */
    static FrameTextureType GetFrameTextureType(const int i);
    /**
     * @brief Convert from proto texture frame to frame texture type.
     * @param texture_frame: Proto texture frame.
     * @return Frame Texture type in local system.
     */
    static FrameTextureType GetFrameTextureType(
        frame::proto::TextureFrame texture_frame);

  public:
    /**
     * @brief From bind interface this return the id of the internal OpenGL
     * object.
     * @return Id of the OpenGL object.
     */
    unsigned int GetId() const final
    {
        return frame_id_;
    }
    //! @brief Lock the bind for RAII interface to the bind interface.
    void LockedBind() const final
    {
        locked_bind_ = true;
    }
    //! @brief Unlock the bind for RAII interface to the bind interface.
    void UnlockedBind() const final
    {
        locked_bind_ = false;
    }

  protected:
    // This was here from the error interface (not used anymore) but it is
    // still used inside.
    const std::pair<bool, std::string> GetError() const;
    // Convert from the internal frame texture type to the OpenGL type.
    const int GetFrameTextureType(
        const FrameTextureType frame_texture_type) const;

  protected:
    friend class ScopedBind;

  private:
    unsigned int frame_id_ = 0;
    mutable bool locked_bind_ = false;
    const Logger& logger_ = Logger::GetInstance();
};

} // End namespace frame::opengl.
