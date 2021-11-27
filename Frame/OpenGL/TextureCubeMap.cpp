#include "TextureCubeMap.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <GL/glew.h>
#include <cassert>
#include "Frame/Level.h"
#include "Frame/OpenGL/Pixel.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/OpenGL/File/LoadProgram.h"

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

	TextureCubeMap::~TextureCubeMap()
	{
		glDeleteTextures(1, &texture_id_);
	}

	TextureCubeMap::TextureCubeMap(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const proto::PixelElementSize pixel_element_size
			/* = PixelElementSize::BYTE*/,
		const proto::PixelStructure pixel_structure
			/* = PixelStructure::RGB*/) :
		TextureCubeMap(pixel_element_size, pixel_structure)
	{
		size_ = size;
		CreateTextureCubeMap();
	}

	TextureCubeMap::TextureCubeMap(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const std::array<void*, 6> cube_data,
		const proto::PixelElementSize pixel_element_size 
			/* = proto::PixelElementSize_BYTE()*/,
		const proto::PixelStructure pixel_structure 
			/* = proto::PixelStructure_RGB()*/) :
		TextureCubeMap(pixel_element_size, pixel_structure)
	{
		size_ = size;
		CreateTextureCubeMap(cube_data);
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
		error_.Display(__FILE__, __LINE__ - 4);
		UnBind();
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

	void TextureCubeMap::CreateTextureCubeMap(
		const std::array<void*, 6> cube_map/* =
			{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }*/)
	{
		glGenTextures(1, &texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		ScopedBind scoped_bind(*this);
		SetMinFilter(proto::Texture::LINEAR);
		SetMagFilter(proto::Texture::LINEAR);
		SetWrapS(proto::Texture::CLAMP_TO_EDGE);
		SetWrapT(proto::Texture::CLAMP_TO_EDGE);
		SetWrapR(proto::Texture::CLAMP_TO_EDGE);
		for (unsigned int i : { 0, 1, 2, 3, 4, 5 })
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
				cube_map[i]);
			error_.Display(__FILE__, __LINE__ - 10);
		}
	}

	int TextureCubeMap::ConvertToGLType(
		const TextureFilterEnum texture_filter) const
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
		}
		throw std::runtime_error(
			"Invalid texture filter : " +
			std::to_string(static_cast<int>(texture_filter)));
	}

	TextureInterface::TextureFilterEnum TextureCubeMap::ConvertFromGLType(
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

	void TextureCubeMap::CreateFrameAndRenderBuffer()
	{
		frame_ = std::make_unique<FrameBuffer>();
		render_ = std::make_unique<RenderBuffer>();
		render_->CreateStorage(size_);
		frame_->AttachRender(*render_);
		frame_->AttachTexture(GetId());
		frame_->DrawBuffers(1);
	}

	void TextureCubeMap::Clear(const glm::vec4 color)
	{
		// First time this is called this will create a frame and a render.
		Bind();
		if (!frame_) CreateFrameAndRenderBuffer();
		ScopedBind scoped_frame(*frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		GLfloat clear_color[4] = { color.r, color.g, color.b, color.a };
		glClearBufferfv(GL_COLOR, 0, clear_color);
		error_.Display(__FILE__, __LINE__ - 1);
		UnBind();
	}

	std::pair<void*, std::size_t> TextureCubeMap::GetTexture(int i) const
	{
		std::pair<void*, std::size_t> result_pair = { nullptr, 0 };
		Bind();
		// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		// error_.Display(__FILE__, __LINE__ - 1);
		auto format = opengl::ConvertToGLType(pixel_structure_);
		auto type = opengl::ConvertToGLType(pixel_element_size_);
		std::vector<std::any> any_vec = {};
		auto size = GetSize();
		auto pixel_structure = GetPixelStructure();
		std::size_t image_size =
			static_cast<std::size_t>(size.first) *
			static_cast<std::size_t>(size.second) *
			static_cast<std::size_t>(pixel_structure);
		short target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
		switch (GetPixelElementSize())
		{
		case proto::PixelElementSize::BYTE:
		{
			result_pair.first = new std::uint8_t[image_size];
			result_pair.second = image_size * sizeof(std::uint8_t);
		}
		break;
		case proto::PixelElementSize::SHORT:
		{
			result_pair.first = new std::uint16_t[image_size];
			result_pair.second = image_size * sizeof(std::uint16_t);
		}
		break;
		case proto::PixelElementSize::HALF:
		{
			result_pair.first = new std::uint16_t[image_size];
			result_pair.second = image_size * sizeof(std::uint16_t);
		}
		break;
		case proto::PixelElementSize::FLOAT:
		{
			result_pair.first = new float[image_size];
			result_pair.second = image_size * sizeof(float);
		}
		break;
		default:
			throw std::runtime_error("Unknown pixel element structure.");
		}
		if (result_pair.second)
		{
			glGetTexImage(target, 0, format, type, result_pair.first);
			error_.Display(__FILE__, __LINE__ - 1);
		}
		UnBind();
		return result_pair;
	}

} // End namespace frame::opengl.