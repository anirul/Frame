#pragma once

#include <memory>
#include "Frame/Error.h"
#include "Frame/Logger.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/OpenGL/ScopedBind.h"

namespace frame::opengl {

	class Texture;

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

	class FrameBuffer : public BindInterface
	{
	public:
		FrameBuffer();
		virtual ~FrameBuffer();

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		// /!\ This will bind and unbind!
		void AttachRender(const RenderBuffer& render) const;
		// /!\ This will bind and unbind!
		void AttachTexture(
			const Texture& texture,
			const FrameColorAttachment frame_color_attachment =
				FrameColorAttachment::COLOR_ATTACHMENT0,
			const int mipmap = 0,
			const FrameTextureType frame_texture_type = 
				FrameTextureType::TEXTURE_2D) const;
		static FrameColorAttachment GetFrameColorAttachment(const int i);
		static FrameTextureType GetFrameTextureType(const int i);
		// /!\ This will bind and unbind!
		void DrawBuffers(const std::uint32_t size = 1);
		// /!\ This will bind and unbind!
		const std::pair<bool, std::string> GetError() const;
		// /!\ This will bind and unbind!
		const std::string GetStatus() const;

	public:
		const unsigned int GetId() const { return frame_id_; }

	protected:
		const int GetFrameTextureType(
			const FrameTextureType frame_texture_type) const;

	protected:
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }

	protected:
		friend class ScopedBind;

	private:
		unsigned int frame_id_ = 0;
		mutable bool locked_bind_ = false;
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::opengl.
