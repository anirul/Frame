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
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	void Texture::SetMagFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	void Texture::SetWrapS(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	void Texture::SetWrapT(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	TextureManager::~TextureManager()
	{
		DisableAll();
	}

	bool TextureManager::AddTexture(
		const std::string& name, 
		const std::shared_ptr<sgl::Texture>& texture)
	{
		RemoveTexture(name);
		auto ret = name_texture_map_.insert({ name, texture });
		return ret.second;
	}

	const std::shared_ptr<sgl::Texture>& TextureManager::GetTexture(
		const std::string& name) const
	{
		auto it = name_texture_map_.find(name);
		if (it == name_texture_map_.end())
		{
			throw std::runtime_error("No such texture: " + name);
		}
		return it->second;
	}

	bool TextureManager::HasTexture(const std::string& name) const
	{
		return name_texture_map_.find(name) != name_texture_map_.end();
	}

	bool TextureManager::RemoveTexture(const std::string& name)
	{
		auto it = name_texture_map_.find(name);
		if (it == name_texture_map_.end())
		{
			return false;
		}
		name_texture_map_.erase(it);
		return true;
	}

	const int TextureManager::EnableTexture(const std::string& name) const
	{
		auto it1 = name_texture_map_.find(name);
		if (it1 == name_texture_map_.end())
		{
			throw std::runtime_error("try to enable a texture: " + name);
		}
		for (int i = 0; i < name_array_.size(); ++i)
		{
			if (name_array_[i].empty())
			{
				name_array_[i] = name;
				it1->second->Bind(i);
				return i;
			}
		}
		throw std::runtime_error("No free slots!");
	}

	const std::vector<std::string> TextureManager::GetTexturesNames() const
	{
		std::vector<std::string> vec = {};
		for (const auto& p : name_texture_map_)
		{
			vec.push_back(p.first);
		}
		return vec;
	}

	void TextureManager::DisableTexture(const std::string& name) const
	{
		auto it1 = name_texture_map_.find(name);
		if (it1 == name_texture_map_.end())
		{
			throw std::runtime_error("no texture named: " + name);
		}
		auto it2 = std::find_if(
			name_array_.begin(),
			name_array_.end(),
			[name](const std::string& value)
		{
			return value == name;
		});
		if (it2 != name_array_.end())
		{
			*it2 = "";
			it1->second->UnBind();
		}
		else
		{
			throw std::runtime_error("No slot bind to: " + name);
		}
	}

	void TextureManager::DisableAll() const
	{
		for (int i = 0; i < 32; ++i)
		{
			if (!name_array_[i].empty())
			{
				DisableTexture(name_array_[i]);
			}
		}
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
		TextureManager texture_manager{};
		Frame frame{};
		Render render{};
		size_ = size;
		auto equirectangular = std::make_shared<Texture>(
			file_name,
			pixel_element_size_,
			pixel_structure_);
		texture_manager.AddTexture(
			"Equirectangular", 
			equirectangular);
		frame.BindAttach(render);
		render.BindStorage(size_);
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
		auto program = CreateProgram("EquirectangularCubeMap");
		program->UniformMatrix("projection", projection);
		auto cube = CreateCubeMesh(program);
		cube->SetTextures({ "Equirectangular" });
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		int i = 0;
		for (glm::mat4 view : views_cubemap)
		{
			frame.BindTexture(
				*this,
				FrameColorAttachment::COLOR_ATTACHMENT0,
				0,
				static_cast<FrameTextureType>(i));
			i++;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
			// TODO(anirul): change this as the view is passed to the program
			// and not the view passed as a projection.
			cube->Draw(texture_manager, view);
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
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	void TextureCubeMap::SetMagFilter(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_MAG_FILTER,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
	}

	void TextureCubeMap::SetWrapS(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_S,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);

	}

	void TextureCubeMap::SetWrapT(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_T,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);

	}

	void TextureCubeMap::SetWrapR(TextureFilter texture_filter)
	{
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_WRAP_R,
			static_cast<GLenum>(texture_filter));
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
		return static_cast<TextureFilter>(filter);
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

	std::shared_ptr<sgl::Texture> TextureBrightness(
		const std::shared_ptr<sgl::Texture>& texture)
	{
		const sgl::Error& error = sgl::Error::GetInstance();
		sgl::Frame frame{};
		sgl::Render render{};
		auto size = texture->GetSize();
		frame.BindAttach(render);
		render.BindStorage(size);
		auto pixel_element_size = texture->GetPixelElementSize();

		auto texture_out = std::make_shared<sgl::Texture>(
			std::pair{ size.first / 2, size.second / 2 },
			pixel_element_size);

		// Set the view port for rendering.
		glViewport(0, 0, size.first / 2, size.second / 2);
		error.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error.Display(__FILE__, __LINE__ - 1);

		frame.BindTexture(*texture_out);

		sgl::TextureManager texture_manager;
		texture_manager.AddTexture("Display", texture);
		auto program = sgl::CreateProgram("Brightness");
		auto quad = sgl::CreateQuadMesh(program);
		quad->SetTextures({ "Display" });
		quad->Draw(texture_manager);

		return texture_out;
	}

	std::shared_ptr<Texture> TextureBlur(
		const std::shared_ptr<Texture>& in_texture,
		const float exponent /*= 1.0f*/)
	{
		static auto out_texture = std::make_shared<Texture>(
			in_texture->GetSize(),
			in_texture->GetPixelElementSize());
		TextureManager texture_manager;
		texture_manager.AddTexture("Image", in_texture);
		static auto program = CreateProgram("Blur");
		program->Use();
		program->UniformFloat("exponent", exponent);
		FillProgramMultiTexture(
			std::vector<std::shared_ptr<Texture>>{ out_texture }, 
			texture_manager, 
			std::vector<std::string>{ "Image" }, 
			program);
		return out_texture;
	}

	std::shared_ptr<Texture> TextureGaussianBlur(
		const std::shared_ptr<Texture>& in_texture)
	{
		const sgl::Error& error = sgl::Error::GetInstance();
		sgl::Frame frame[2];
		sgl::Render render{};
		auto size = in_texture->GetSize();
		frame[0].BindAttach(render);
		frame[1].BindAttach(render);
		render.BindStorage(size);
		auto pixel_element_size = in_texture->GetPixelElementSize();

		std::shared_ptr<sgl::Texture> texture_out[2] = {
			std::make_shared<sgl::Texture>(
				size,
				pixel_element_size),
			std::make_shared<sgl::Texture>(
				size,
				pixel_element_size)
		};

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error.Display(__FILE__, __LINE__ - 1);

		frame[0].BindTexture(*texture_out[0]);
		frame[1].BindTexture(*texture_out[1]);

		static auto program = CreateProgram("GaussianBlur");
		static auto quad = CreateQuadMesh(program);
		program->Use();

		bool horizontal = true;
		bool first_iteration = true;
		for (int i = 0; i < 10; ++i)
		{
			// Reset the texture manager.
			TextureManager texture_manager;
			program->UniformInt("horizontal", horizontal);
			frame[horizontal].Bind();
			texture_manager.AddTexture(
				"Image",
				(first_iteration) ? in_texture : texture_out[!horizontal]);
			quad->SetTextures({ "Image" });
			quad->Draw(texture_manager);
			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}

		return texture_out[!horizontal];
	}

	std::shared_ptr<sgl::Texture> TextureAddition(
		const std::vector<std::shared_ptr<sgl::Texture>>& add_textures)
	{
		assert(add_textures.size() <= 16);
		const sgl::Error& error = sgl::Error::GetInstance();
		sgl::Frame frame{};
		sgl::Render render{};
		auto size = add_textures[0]->GetSize();
		frame.BindAttach(render);
		render.BindStorage(size);

		auto texture_out = std::make_shared<sgl::Texture>(
			size,
			add_textures[0]->GetPixelElementSize());

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error.Display(__FILE__, __LINE__ - 1);

		frame.BindTexture(*texture_out);
		frame.DrawBuffers(1);

		sgl::TextureManager texture_manager;
		static auto program = sgl::CreateProgram("VectorAddition");
		static auto quad = sgl::CreateQuadMesh(program);
		int i = 0;
		std::vector<std::string> vec = {};
		for (const auto& texture : add_textures)
		{
			texture_manager.AddTexture("Texture" + std::to_string(i), texture);
			vec.push_back("Texture" + std::to_string(i));
			i++;
		}
		program->Use();
		program->UniformInt(
			"texture_max", 
			static_cast<int>(add_textures.size()));
		quad->SetTextures(vec);
		quad->Draw(texture_manager);

		return texture_out;
	}

	std::shared_ptr<Texture> TextureMultiply(
		const std::vector<std::shared_ptr<Texture>>& multiply_textures)
	{
		assert(multiply_textures.size() <= 16);
		const sgl::Error& error = sgl::Error::GetInstance();
		sgl::Frame frame{};
		sgl::Render render{};
		auto size = multiply_textures[0]->GetSize();
		frame.BindAttach(render);
		render.BindStorage(size);

		auto texture_out = std::make_shared<sgl::Texture>(
			size,
			multiply_textures[0]->GetPixelElementSize());

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error.Display(__FILE__, __LINE__ - 1);

		frame.BindTexture(*texture_out);
		frame.DrawBuffers(1);

		sgl::TextureManager texture_manager;
		static auto program = sgl::CreateProgram("VectorMultiply");
		static auto quad = sgl::CreateQuadMesh(program);
		int i = 0;
		std::vector<std::string> vec = {};
		for (const auto& texture : multiply_textures)
		{
			texture_manager.AddTexture("Texture" + std::to_string(i), texture);
			vec.push_back("Texture" + std::to_string(i));
			i++;
		}
		program->Use();
		program->UniformInt(
			"texture_max",
			static_cast<int>(multiply_textures.size()));
		quad->SetTextures(vec);
		quad->Draw(texture_manager);

		return texture_out;
	}

	void FillProgramMultiTexture(
		std::vector<std::shared_ptr<Texture>>& out_textures, 
		const TextureManager& texture_manager, 
		const std::vector<std::string>& texture_selected, 
		const std::shared_ptr<Program>& program)
	{
		FillProgramMultiTextureMipmap(
			out_textures,
			texture_manager,
			texture_selected,
			program,
			0,
			[](const int, const std::shared_ptr<Program>&) {});
	}

	void FillProgramMultiTextureMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<Program>& program)> func /*=
				[](const int, const std::shared_ptr<sgl::Program>&) {}*/)
	{
		auto& error = Error::GetInstance();
		assert(out_textures.size());
		auto size = out_textures[0]->GetSize();
		Frame frame{};
		Render render{};
		frame.BindAttach(render);
		render.BindStorage(size);
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1) {
			for (const auto& texture : out_textures)
			{
				texture->BindEnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
		auto quad = CreateQuadMesh(program);
		quad->SetTextures(texture_selected);
		std::pair<uint32_t, uint32_t> temporary_size = size;
		for (int mipmap_level = 0; mipmap_level < max_mipmap; ++mipmap_level)
		{
			func(mipmap_level, program);
			double fact = std::pow(0.5, mipmap_level);
			temporary_size.first =
				static_cast<std::uint32_t>(size.first * fact);
			temporary_size.second =
				static_cast<std::uint32_t>(size.second * fact);
			glViewport(0, 0, temporary_size.first, temporary_size.second);
			error.Display(__FILE__, __LINE__ - 1);
			for (int i = 0; i < out_textures.size(); ++i)
			{
				frame.BindTexture(
					*out_textures[i], 
					Frame::GetFrameColorAttachment(i),
					mipmap_level);
			}
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			error.Display(__FILE__, __LINE__ - 1);
			quad->Draw(texture_manager, projection);
		}
	}

	void FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<Texture>>& out_textures, 
		const TextureManager& texture_manager, 
		const std::vector<std::string>& texture_selected, 
		const std::shared_ptr<Program>& program)
	{
		FillProgramMultiTextureCubeMapMipmap(
			out_textures,
			texture_manager,
			texture_selected,
			program,
			0,
			[](const int, const std::shared_ptr<Program>&) {});
	}

	void FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<sgl::Program>& program)> func /*=
				[](const int, const std::shared_ptr<sgl::Program>&) {}*/)
	{
		auto& error = Error::GetInstance();
		assert(out_textures.size());
		auto size = out_textures[0]->GetSize();
		Frame frame{};
		Render render{};
		frame.BindAttach(render);
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1) 
		{
			for (const auto& texture : out_textures)
			{
				texture->BindEnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
		auto cube = CreateCubeMesh(program);
		cube->SetTextures(texture_selected);
		std::pair<std::uint32_t, std::uint32_t> temporary_size = { 0, 0 };
		for (int mipmap_level = 0; mipmap_level < max_mipmap; ++mipmap_level)
		{
			func(mipmap_level, program);
			double fact = std::pow(0.5, mipmap_level);
			temporary_size.first =
				static_cast<std::uint32_t>(size.first * fact);
			temporary_size.second =
				static_cast<std::uint32_t>(size.second * fact);
			render.BindStorage(temporary_size);
			glViewport(0, 0, temporary_size.first, temporary_size.second);
			error.Display(__FILE__, __LINE__ - 1);
			int cubemap_element = 0;
			for (glm::mat4 view : views_cubemap)
			{
				for (int i = 0; i < out_textures.size(); ++i)
				{
					frame.BindTexture(
						*out_textures[i],
						Frame::GetFrameColorAttachment(i),
						mipmap_level,
						Frame::GetFrameTextureType(cubemap_element));
				}
				cubemap_element++;
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				error.Display(__FILE__, __LINE__ - 1);
				cube->Draw(texture_manager, projection, view);
			}
		}
	}
		
} // End namespace sgl.
