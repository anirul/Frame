#include "Texture.h"
#include <GL/glew.h>
#include "Image.h"
#include "Frame.h"
#include "Render.h"
#include "Program.h"
#include "Mesh.h"

namespace sgl {

	Texture::Texture(
		const std::string& file, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		sgl::Image img(file, pixel_element_size_, pixel_structure_);
		size_ = img.GetSize();
		glGenTextures(1, &texture_id_);
		if (texture_id_ == 0)
		{
			throw std::runtime_error("Unable to create texture.");
		}
		glBindTexture(GL_TEXTURE_2D, texture_id_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
		error_->DisplayError(__FILE__, __LINE__);
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &texture_id_);
		// error_->DisplayError(__FILE__, __LINE__);
	}

	void Texture::Bind(const unsigned int slot /*= 0*/) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		error_->DisplayError(__FILE__, __LINE__);
		glBindTexture(GL_TEXTURE_2D, texture_id_);
		error_->DisplayError(__FILE__, __LINE__);
	}

	void Texture::UnBind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		error_->DisplayError(__FILE__, __LINE__);
	}

	void Texture::BindEnableMipmap() const
	{
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		error_->DisplayError(__FILE__, __LINE__);
	}

	TextureManager::~TextureManager()
	{
		DisableAll();
	}

	bool TextureManager::AddTexture(
		const std::string& name, 
		const std::shared_ptr<sgl::Texture>& texture)
	{
		auto ret = name_texture_map_.insert({ name, texture });
		return ret.second;
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
		CreateCubeMap();
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
			error_->DisplayError(__FILE__, __LINE__);
		}
	}

	TextureCubeMap::TextureCubeMap(
		const std::string& file_name, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		Texture(pixel_element_size, pixel_structure)
	{
		TextureManager texture_manager{};
		texture_manager.AddTexture(
			"Equirectangular", 
			std::make_shared<Texture>(
				file_name,
				pixel_element_size_,
				pixel_structure_));
		Frame frame{};
		Render render{};
		frame.BindAttach(render);
		render.BindStorage(std::make_pair(512, 512));
		CreateCubeMap();
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
				0, 
				ConvertToGLType(pixel_element_size_, pixel_structure_),
				512, 
				512, 
				0, 
				ConvertToGLType(pixel_structure_), 
				ConvertToGLType(pixel_element_size_), 
				nullptr);
			error_->DisplayError(__FILE__, __LINE__);
		}
		glm::mat4 projection =
			glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		std::shared_ptr<Program> program = 
			CreateEquirectangulareCubeMapProgram(projection);
		Mesh cube("../Asset/Cube.obj", program);
		cube.SetTextures({ "Equirectangular" });
		glViewport(0, 0, 512, 512);
		error_->DisplayError(__FILE__, __LINE__);
		Bind(texture_manager.EnableTexture("Equirectangular"));
		int i = 0;
		for (glm::mat4 view : views_)
		{
			frame.BindTexture2D(
				*this, 
				static_cast<FrameTextureType>(i));
			i++;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cube.Draw(texture_manager, view);
		}
	}

	void TextureCubeMap::Bind(const unsigned int slot /*= 0*/) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		error_->DisplayError(__FILE__, __LINE__);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
		error_->DisplayError(__FILE__, __LINE__);
	}

	void TextureCubeMap::UnBind() const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		error_->DisplayError(__FILE__, __LINE__);
	}

	void TextureCubeMap::BindEnableMipmap() const
	{
		Bind();
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		error_->DisplayError(__FILE__, __LINE__);
	}

	void TextureCubeMap::CreateCubeMap()
	{
		glGenTextures(1, &texture_id_);
		if (texture_id_ == 0)
		{
			throw std::runtime_error("Unable to create texture.");
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP, 
			GL_TEXTURE_WRAP_S, 
			GL_CLAMP_TO_EDGE);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP, 
			GL_TEXTURE_WRAP_T, 
			GL_CLAMP_TO_EDGE);
		glTexParameteri(
			GL_TEXTURE_CUBE_MAP, 
			GL_TEXTURE_WRAP_R, 
			GL_CLAMP_TO_EDGE);
		error_->DisplayError(__FILE__, __LINE__);
	}

} // End namespace sgl.
