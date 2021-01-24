#include "Texture.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <GL/glew.h>
#include <cassert>
#include "FrameBuffer.h"
#include "RenderBuffer.h"
#include "Program.h"
#include "StaticMesh.h"

namespace frame::opengl {

	namespace {
		// Get the 6 view for the cube map.
		const std::array<glm::mat4, 6> views_cubemap =
		{
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(-1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f))
		};
	}

	Texture::Texture(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const proto::PixelElementSize& pixel_element_size 
			/*= PixelElementSize::BYTE*/, 
		const proto::PixelStructure& pixel_structure 
			/*= PixelStructure::RGB*/) :
		size_(size),
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		CreateTexture();
		Bind();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			opengl::ConvertToGLType(pixel_structure_),
			opengl::ConvertToGLType(pixel_element_size_),
			nullptr);
		error_.Display(__FILE__, __LINE__ - 10);
		UnBind();
	}

	Texture::Texture(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const void* data, 
		const proto::PixelElementSize& pixel_element_size
			/*= PixelElementSize::BYTE*/, 
		const proto::PixelStructure& pixel_structure 
			/*= PixelStructure::RGB*/) :
		size_(size),
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		CreateTexture();
		Bind();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			opengl::ConvertToGLType(pixel_structure_),
			opengl::ConvertToGLType(pixel_element_size_),
			data);
		error_.Display(__FILE__, __LINE__ - 10);
		UnBind();
	}

	void Texture::CreateTexture()
	{
		glGenTextures(1, &texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		error_.Display(__FILE__, __LINE__ - 1);
		UnBind();
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &texture_id_);
	}

	void Texture::Bind(const unsigned int slot /*= 0*/) const
	{
		if (locked_bind_) return;
		assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		glActiveTexture(GL_TEXTURE0 + slot);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_2D, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::UnBind() const
	{
		if (locked_bind_) return;
		glBindTexture(GL_TEXTURE_2D, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::EnableMipmap() const
	{
		glGenerateMipmap(GL_TEXTURE_2D);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::SetMinFilter(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_2D, 
			GL_TEXTURE_MIN_FILTER, 
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum Texture::GetMinFilter() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void Texture::SetMagFilter(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			ConvertToGLType(texture_filter));
		UnBind();
		error_.Display(__FILE__, __LINE__ - 4);
	}

	TextureInterface::TextureFilterEnum Texture::GetMagFilter() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void Texture::SetWrapS(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum Texture::GetWrapS() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void Texture::SetWrapT(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum Texture::GetWrapT() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void Texture::Clear(const glm::vec4 color)
	{
		// First time this is called this will create a frame and a render.
		Bind();
		if (!frame_)
		{
			frame_ = std::make_shared<FrameBuffer>();
			render_ = std::make_shared<RenderBuffer>();
			render_->CreateStorage(size_);
			frame_->AttachRender(*render_);
			frame_->AttachTexture(shared_from_this());
			frame_->DrawBuffers(1);
		}
		ScopedBind scoped_frame(*frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		GLfloat clear_color[4] = { color.r, color.g, color.b, color.a };
		glClearBufferfv(GL_COLOR, 0, clear_color);
		error_.Display(__FILE__, __LINE__ - 1);
		UnBind();
	}

	TextureCubeMap::TextureCubeMap(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const proto::PixelElementSize& pixel_element_size 
			/*= PixelElementSize::BYTE*/, 
		const proto::PixelStructure& pixel_structure 
			/*= PixelStructure::RGB*/) :
		Texture(pixel_element_size, pixel_structure)
	{
		size_ = size;
		CreateTextureCubeMap();
		Bind();
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0,
				opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
				static_cast<GLsizei>(size_.first),
				static_cast<GLsizei>(size_.second),
				0,
				opengl::ConvertToGLType(pixel_structure_),
				opengl::ConvertToGLType(pixel_element_size_),
				nullptr);
			error_.Display(__FILE__, __LINE__ - 10);
		}
		UnBind();
	}

	TextureCubeMap::TextureCubeMap(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const void* data,
		const proto::PixelElementSize& pixel_element_size /*=
			proto::PixelElementSize_BYTE()*/,
		const proto::PixelStructure& pixel_structure /*=
			proto::PixelStructure_RGB()*/)
	{
		throw std::runtime_error("Not implemented!");
	}

	TextureCubeMap::TextureCubeMap(
		const std::array<void*, 6>& cube_data,
		const proto::PixelElementSize& pixel_element_size /*=
			proto::PixelElementSize_BYTE()*/,
		const proto::PixelStructure& pixel_structure /*=
			proto::PixelStructure_RGB()*/)
	{
		throw std::runtime_error("Not implemented!");
	}

	void TextureCubeMap::Bind(const unsigned int slot /*= 0*/) const
	{
		assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		if (locked_bind_) return;
		glActiveTexture(GL_TEXTURE0 + slot);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::UnBind() const
	{
		if (locked_bind_) return;
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::EnableMipmap() const
	{
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::SetMinFilter(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MIN_FILTER,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::GetMinFilter() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MIN_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::SetMagFilter(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MAG_FILTER,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::GetMagFilter() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MAG_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::SetWrapS(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			ConvertToGLType(texture_filter));
		UnBind();
		error_.Display(__FILE__, __LINE__ - 4);
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::GetWrapS() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);

	}

	void TextureCubeMap::SetWrapT(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::GetWrapT() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);

	}

	void TextureCubeMap::SetWrapR(const TextureFilterEnum texture_filter)
	{
		Bind();
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::GetWrapR() const
	{
		GLint filter;
		Bind();
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::CreateTextureCubeMap()
	{
		glGenTextures(1, &texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		Bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		error_.Display(__FILE__, __LINE__ - 1);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 4);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 4);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			GL_CLAMP_TO_EDGE);
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
	}

	int Texture::ConvertToGLType(const TextureFilterEnum texture_filter) const
	{
		switch (texture_filter)
		{
			case frame::proto::Texture::NEAREST:
				return GL_NEAREST;
			case frame::proto::Texture::LINEAR:
				return GL_LINEAR;
			case frame::proto::Texture::NEAREST_MIPMAP_NEAREST:
				return GL_NEAREST_MIPMAP_NEAREST;
			case frame::proto::Texture::LINEAR_MIPMAP_NEAREST:
				return GL_LINEAR_MIPMAP_NEAREST;
			case frame::proto::Texture::NEAREST_MIPMAP_LINEAR:
				return GL_NEAREST_MIPMAP_LINEAR;
			case frame::proto::Texture::LINEAR_MIPMAP_LINEAR:
				return GL_LINEAR_MIPMAP_LINEAR;
			case frame::proto::Texture::CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
			case frame::proto::Texture::MIRRORED_REPEAT:
				return GL_MIRRORED_REPEAT;
			case frame::proto::Texture::REPEAT:
				return GL_REPEAT;
			default:
				throw
					std::runtime_error(
						"Invalid texture filter : " +
						std::to_string(
							static_cast<int>(texture_filter)));
		}
	}

	TextureInterface::TextureFilterEnum Texture::ConvertFromGLType(
		int gl_filter) const
	{
		switch (gl_filter)
		{
		case GL_NEAREST:
			return frame::proto::Texture::NEAREST;
		case GL_LINEAR:
			return frame::proto::Texture::LINEAR;
		case GL_NEAREST_MIPMAP_NEAREST:
			return frame::proto::Texture::NEAREST_MIPMAP_NEAREST;
		case GL_LINEAR_MIPMAP_NEAREST:
			return frame::proto::Texture::LINEAR_MIPMAP_NEAREST;
		case GL_NEAREST_MIPMAP_LINEAR:
			return frame::proto::Texture::NEAREST_MIPMAP_LINEAR;
		case GL_LINEAR_MIPMAP_LINEAR:
			return frame::proto::Texture::LINEAR_MIPMAP_LINEAR;
		case GL_CLAMP_TO_EDGE:
			return frame::proto::Texture::CLAMP_TO_EDGE;
		case GL_MIRRORED_REPEAT:
			return frame::proto::Texture::MIRRORED_REPEAT;
		case GL_REPEAT:
			return frame::proto::Texture::REPEAT;
		}
		throw std::runtime_error(
			"invalid texture filter : " + std::to_string(gl_filter));
	}

} // End namespace frame::opengl.