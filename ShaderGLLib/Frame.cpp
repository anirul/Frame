#include "Frame.h"
#include <stdexcept>
#include <GL/glew.h>

namespace sgl {

	Frame::Frame()
	{
		glGenFramebuffers(1, &frame_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	Frame::~Frame()
	{
		glDeleteFramebuffers(1, &frame_id_);
	}

	void Frame::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frame_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Frame::UnBind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Frame::BindAttach(const Render& render) const
	{
		Bind();
		render.Bind();
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, 
			GL_DEPTH_ATTACHMENT, 
			GL_RENDERBUFFER, 
			render.GetId());
		error_.Display(__FILE__, __LINE__ - 5);
	}

	void Frame::BindTexture(
		const Texture& texture,
		const FrameColorAttachment frame_color_attachment /*=
			FrameColorAttachment::COLOR_ATTACHMENT0*/,
		const int mipmap /*= 0*/,
		const FrameTextureType frame_texture_type /*= 
			FrameTextureType::TEXTURE_2D*/) const
	{
		Bind();
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			static_cast<GLenum>(frame_color_attachment),
			GetFrameTextureType(frame_texture_type),
			texture.GetId(),
			mipmap);
		error_.Display(__FILE__, __LINE__ - 6);
	}

	FrameColorAttachment Frame::GetFrameColorAttachment(const int i)
	{
		switch (i)
		{
			case 0:
				return FrameColorAttachment::COLOR_ATTACHMENT0;
			case 1:
				return FrameColorAttachment::COLOR_ATTACHMENT1;
			case 2:
				return FrameColorAttachment::COLOR_ATTACHMENT2;
			case 3:
				return FrameColorAttachment::COLOR_ATTACHMENT3;
			case 4:
				return FrameColorAttachment::COLOR_ATTACHMENT4;
			case 5:
				return FrameColorAttachment::COLOR_ATTACHMENT5;
			case 6:
				return FrameColorAttachment::COLOR_ATTACHMENT6;
			case 7:
				return FrameColorAttachment::COLOR_ATTACHMENT7;
			default :
				throw std::runtime_error("Only [0-7] level allowed.");
		}
	}

	const int Frame::GetFrameTextureType(
		const FrameTextureType frame_texture_type) const
	{
		int value = static_cast<int>(frame_texture_type);
		if (value >= 0)
		{
			return GL_TEXTURE_CUBE_MAP_POSITIVE_X + value;
		}
		return GL_TEXTURE_2D;
	}

	FrameTextureType Frame::GetFrameTextureType(const int i)
	{
		return static_cast<FrameTextureType>(i);
	}

} // End namespace sgl.
