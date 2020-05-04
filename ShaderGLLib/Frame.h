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
			const int mipmap = 0,
			const FrameTextureType frame_texture_type = 
				FrameTextureType::TEXTURE_2D) const;

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
