#pragma once

#include <memory>
#include "../ShaderGLLib/Render.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	enum class FrameTextureType
	{
		TEXTURE_2D = -1,
		CUBE_MAP_POSITIVE_X = 0,
		CUBE_MAP_NEGATIVE_X = 1,
		CUBE_MAP_POSITIVE_Y = 2,
		CUBE_MAP_NEGATIVE_Y = 3,
		CUBE_MAP_POSITIVE_Z = 4,
		CUBE_MAP_NEGATIVE_Z = 5,
	};

	enum class FrameColorAttachment
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

	class Frame 
	{
	public:
		Frame();
		virtual ~Frame();

	public:
		void Bind() const;
		void UnBind() const;
		void BindAttach(const Render& render) const;
		void BindTexture(
			const Texture& texture,
			const FrameColorAttachment frame_color_attachment =
				FrameColorAttachment::COLOR_ATTACHMENT0,
			const int mipmap = 0,
			const FrameTextureType frame_texture_type = 
				FrameTextureType::TEXTURE_2D) const;
		static FrameColorAttachment GetFrameColorAttachment(const int i);
		static FrameTextureType GetFrameTextureType(const int i);

	public:
		const unsigned int GetId() const { return frame_id_; }

	protected:
		const int GetFrameTextureType(
			const FrameTextureType frame_texture_type) const;

	protected:
		unsigned int frame_id_ = 0;
		const Error& error_ = Error::GetInstance();
	};

} // End namespace sgl.
