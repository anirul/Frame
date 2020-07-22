#include "Texture.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <GL/glew.h>
#include <cassert>
#include "Image.h"
#include "Frame.h"
#include "Render.h"
#include "Program.h"
#include "Mesh.h"

namespace sgl {

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
		const std::string& file, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		sgl::Image img(file, pixel_element_size_, pixel_structure_);
		size_ = img.GetSize();
		CreateTexture();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			sgl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			sgl::ConvertToGLType(pixel_structure_),
			sgl::ConvertToGLType(pixel_element_size_),
			img.Data());
		error_.Display(__FILE__, __LINE__ - 10);
	}

	Texture::Texture(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		size_(size),
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		CreateTexture();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			sgl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			sgl::ConvertToGLType(pixel_structure_),
			sgl::ConvertToGLType(pixel_element_size_),
			nullptr);
		error_.Display(__FILE__, __LINE__ - 10);
	}

	Texture::Texture(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const void* data, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		size_(size),
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		CreateTexture();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			sgl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			sgl::ConvertToGLType(pixel_structure_),
			sgl::ConvertToGLType(pixel_element_size_),
			data);
		error_.Display(__FILE__, __LINE__ - 10);
	}

	Texture::Texture(const frame::proto::Texture& texture) :
		pixel_element_size_(texture.pixel_element_size()),
		pixel_structure_(texture.pixel_structure())
	{
		// Get the pixel element size.
		if (texture.pixel_element_size().value() == 
			frame::proto::PixelElementSize::INVALID)
		{
			error_.CreateError(
				"Invalid pixel element size.", 
				__FILE__, 
				__LINE__ - 6);
		}
		if (texture.pixel_structure().value() == 
			frame::proto::PixelStructure::INVALID)
		{ 
			error_.CreateError(
				"Invalid pixel structure.",
				__FILE__,
				__LINE__ - 6);
		}
		CreateTexture();
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			sgl::ConvertToGLType(pixel_element_size_, pixel_structure_),
			static_cast<GLsizei>(size_.first),
			static_cast<GLsizei>(size_.second),
			0,
			sgl::ConvertToGLType(pixel_structure_),
			sgl::ConvertToGLType(pixel_element_size_),
			texture.pixels().empty() ? nullptr : texture.pixels().data());
		
	}

	void Texture::CreateTexture()
	{
		glGenTextures(1, &texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_2D, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
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
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &texture_id_);
	}

	void Texture::Bind(const unsigned int slot /*= 0*/) const
	{
		assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		glActiveTexture(GL_TEXTURE0 + slot);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_2D, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::UnBind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::BindEnableMipmap() const
	{
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Texture::SetMinFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D, 
			GL_TEXTURE_MIN_FILTER, 
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	TextureFilter Texture::GetMinFilter() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void Texture::SetMagFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter Texture::GetMagFilter() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void Texture::SetWrapS(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter Texture::GetWrapS() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void Texture::SetWrapT(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter Texture::GetWrapT() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void Texture::Clear(const glm::vec4 color)
	{
		// First time this is called this will create a frame and a render.
		if (!frame_)
		{
			frame_ = std::make_shared<Frame>();
			render_ = std::make_shared<Render>();
			render_->CreateStorage(size_);
			frame_->AttachRender(*render_);
			frame_->AttachTexture(*this);
			frame_->DrawBuffers(1);
		}
		ScopedBind scoped_frame(*frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		GLfloat clear_color[4] = { color.r, color.g, color.b, color.a };
		glClearBufferfv(GL_COLOR, 0, clear_color);
	}

	TextureCubeMap::TextureCubeMap(
		const std::array<std::string, 6>& cube_file,
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/,
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		Texture(pixel_element_size, pixel_structure)
	{
		CreateTextureCubeMap();
		for (const int i : {0, 1, 2, 3, 4, 5})
		{
			sgl::Image image(
				cube_file[i],
				pixel_element_size_,
				pixel_structure_);
			auto size = image.GetSize();
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0,
				sgl::ConvertToGLType(pixel_element_size_, pixel_structure_),
				static_cast<GLsizei>(size.first),
				static_cast<GLsizei>(size.second),
				0,
				sgl::ConvertToGLType(pixel_structure_),
				sgl::ConvertToGLType(pixel_element_size_),
				image.Data());
			error_.Display(__FILE__, __LINE__ - 10);
		}
	}

	TextureCubeMap::TextureCubeMap(
		const std::string& file_name, 
		const std::pair<std::uint32_t, std::uint32_t> size,
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		Texture(pixel_element_size, pixel_structure)
	{
		auto material = std::make_shared<Material>();
		Frame frame{};
		Render render{};
		ScopedBind scoped_frame(frame);
		ScopedBind scoped_render(render);
		size_ = size;
		auto equirectangular = std::make_shared<Texture>(
			file_name,
			pixel_element_size_,
			pixel_structure_);
		material->AddTexture("Equirectangular", equirectangular);
		frame.AttachRender(render);
		render.CreateStorage(size_);
		CreateTextureCubeMap();
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
				0, 
				ConvertToGLType(pixel_element_size_, pixel_structure_),
				static_cast<GLsizei>(size_.first),
				static_cast<GLsizei>(size_.second),
				0, 
				ConvertToGLType(pixel_structure_), 
				ConvertToGLType(pixel_element_size_),
				nullptr);
			error_.Display(__FILE__, __LINE__ - 10);
		}
		glm::mat4 projection =
			glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		auto program = Program::CreateProgram("EquirectangularCubeMap");
		program->UniformMatrix("projection", projection);
		auto cube = CreateCubeMesh(program);
		cube->SetMaterial(material);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		int i = 0;
		for (glm::mat4 view : views_cubemap)
		{
			frame.AttachTexture(
				*this,
				FrameColorAttachment::COLOR_ATTACHMENT0,
				0,
				static_cast<FrameTextureType>(i));
			i++;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
			// TODO(anirul): change this as the view is passed to the program
			// and not the view passed as a projection.
			cube->Draw(view);
		}
	}

	TextureCubeMap::TextureCubeMap(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
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
				ConvertToGLType(pixel_element_size_, pixel_structure_),
				static_cast<GLsizei>(size_.first),
				static_cast<GLsizei>(size_.second),
				0,
				ConvertToGLType(pixel_structure_),
				ConvertToGLType(pixel_element_size_),
				nullptr);
			error_.Display(__FILE__, __LINE__ - 10);
		}
	}

	void TextureCubeMap::Bind(const unsigned int slot /*= 0*/) const
	{
		assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		glActiveTexture(GL_TEXTURE0 + slot);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::UnBind() const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::BindEnableMipmap() const
	{
		Bind();
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void TextureCubeMap::SetMinFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MIN_FILTER,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter TextureCubeMap::GetMinFilter() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MIN_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::SetMagFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MAG_FILTER,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter TextureCubeMap::GetMagFilter() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MAG_FILTER,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::SetWrapS(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter TextureCubeMap::GetWrapS() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);

	}

	void TextureCubeMap::SetWrapT(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter TextureCubeMap::GetWrapT() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);

	}

	void TextureCubeMap::SetWrapR(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			ConvertToGLType(texture_filter));
		error_.Display(__FILE__, __LINE__ - 4);
	}

	sgl::TextureFilter TextureCubeMap::GetWrapR() const
	{
		GLint filter;
		glGetTexParameteriv(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			&filter);
		error_.Display(__FILE__, __LINE__ - 4);
		return ConvertFromGLType(filter);
	}

	void TextureCubeMap::CreateTextureCubeMap()
	{
		glGenTextures(1, &texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
		error_.Display(__FILE__, __LINE__ - 1);
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
	}

	TextureFilter TextureFilter_NEAREST()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::NEAREST);
		return texture_filter;
	}

	TextureFilter TextureFilter_LINEAR()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::LINEAR);
		return texture_filter;
	}

	TextureFilter TextureFilter_NEAREST_MIPMAP_NEAREST()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::NEAREST_MIPMAP_NEAREST);
		return texture_filter;
	}

	TextureFilter TextureFilter_LINEAR_MIPMAP_NEAREST()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::LINEAR_MIPMAP_NEAREST);
		return texture_filter;
	}

	TextureFilter TextureFilter_NEAREST_MIPMAP_LINEAR()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::NEAREST_MIPMAP_LINEAR);
		return texture_filter;
	}

	TextureFilter TextureFilter_LINEAR_MIPMAP_LINEAR()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::LINEAR_MIPMAP_LINEAR);
		return texture_filter;
	}

	TextureFilter TextureFilter_CLAMP_TO_EDGE()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::CLAMP_TO_EDGE);
		return texture_filter;
	}

	TextureFilter TextureFilter_MIRRORED_REPEAT()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::MIRRORED_REPEAT);
		return texture_filter;
	}

	TextureFilter TextureFilter_REPEAT()
	{
		TextureFilter texture_filter{};
		texture_filter.set_value(TextureFilter::REPEAT);
		return texture_filter;
	}

	int ConvertToGLType(const TextureFilter& texture_filter)
	{
		switch (texture_filter.value())
		{
			case TextureFilter::NEAREST:
				return GL_NEAREST;
			case TextureFilter::LINEAR:
				return GL_LINEAR;
			case TextureFilter::NEAREST_MIPMAP_NEAREST:
				return GL_NEAREST_MIPMAP_NEAREST;
			case TextureFilter::LINEAR_MIPMAP_NEAREST:
				return GL_LINEAR_MIPMAP_NEAREST;
			case TextureFilter::NEAREST_MIPMAP_LINEAR:
				return GL_NEAREST_MIPMAP_LINEAR;
			case TextureFilter::LINEAR_MIPMAP_LINEAR:
				return GL_LINEAR_MIPMAP_LINEAR;
			case TextureFilter::CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
			case TextureFilter::MIRRORED_REPEAT:
				return GL_MIRRORED_REPEAT;
			case TextureFilter::REPEAT:
				return GL_REPEAT;
		}
		throw
			std::runtime_error(
				"Invalid texture filter : " +
				std::to_string(static_cast<int>(texture_filter.value())));
	}

	TextureFilter ConvertFromGLType(int gl_filter)
	{
		switch (gl_filter)
		{
		case GL_NEAREST:
			return TextureFilter_NEAREST();
		case GL_LINEAR:
			return TextureFilter_LINEAR();
		case GL_NEAREST_MIPMAP_NEAREST:
			return TextureFilter_NEAREST_MIPMAP_NEAREST();
		case GL_LINEAR_MIPMAP_NEAREST:
			return TextureFilter_LINEAR_MIPMAP_NEAREST();
		case GL_NEAREST_MIPMAP_LINEAR:
			return TextureFilter_NEAREST_MIPMAP_LINEAR();
		case GL_LINEAR_MIPMAP_LINEAR:
			return TextureFilter_LINEAR_MIPMAP_LINEAR();
		case GL_CLAMP_TO_EDGE:
			return TextureFilter_CLAMP_TO_EDGE();
		case GL_MIRRORED_REPEAT:
			return TextureFilter_MIRRORED_REPEAT();
		case GL_REPEAT:
			return TextureFilter_REPEAT();
		}
		throw std::runtime_error(
			"invalid texture filter : " + std::to_string(gl_filter));
	}

} // End namespace sgl.
