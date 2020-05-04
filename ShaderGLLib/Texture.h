#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
#include <map>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"
#include "../ShaderGLLib/Program.h"

namespace sgl {

	class Texture 
	{
	public:
		// Create an empty texture of size size.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		// Create a texture from a file.
		Texture(
			const std::string& file,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		virtual ~Texture();

	public:
		virtual void Bind(const unsigned int slot = 0) const;
		virtual void UnBind() const;
		virtual void BindEnableMipmap() const;

	public:
		void CreateTexture();
		const int GetId() const { return texture_id_; }
		std::pair<std::uint32_t, std::uint32_t> GetSize() const 
		{ 
			return size_; 
		}
		const PixelElementSize GetPixelElementSize() const 
		{
			return pixel_element_size_; 
		}
		const PixelStructure GetPixelStructure() const
		{
			return pixel_structure_;
		}

	protected:
		Texture(
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}

	protected:
		unsigned int texture_id_ = 0;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_;
		const PixelStructure pixel_structure_;
		const Error& error_ = Error::GetInstance();
	};

	class TextureCubeMap : public Texture
	{
	public:
		// Create an empty cubemap of the size size.
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		// Take a single texture and map it to a cube view.
		TextureCubeMap(
			const std::string& file_name,
			const std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 },
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		// Take 6 texture to be mapped to the cube map, Order is:
		// right, left - (positive X, negative X)
		// top, bottom - (positive Y, negative Y)
		// front, back - (positive Z, negative Z)
		// The size is equal to the size of an image (*6).
		TextureCubeMap(
			const std::array<std::string, 6>& cube_file,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		void BindEnableMipmap() const override;

	protected:
		// Create a cube map and assign it to the texture_id_.
		void CreateTextureCubeMap();
	};

	// This texture manager is there to hold texture to the mesh in order to be
	// rendered. So you can have a single texture manager for a large set of
	// meshes.
	class TextureManager 
	{
	public:
		TextureManager() = default;
		virtual ~TextureManager();

	public:
		bool AddTexture(
			const std::string& name, 
			const std::shared_ptr<sgl::Texture>& texture);
		const std::shared_ptr<sgl::Texture>& GetTexture(
			const std::string& name) const;
		bool RemoveTexture(const std::string& name);
		// Return the binding slot of the texture (to be passed to the program).
		const int EnableTexture(const std::string& name) const;
		void DisableTexture(const std::string& name) const;
		void DisableAll() const;

	private:
		std::map<std::string, std::shared_ptr<Texture>> name_texture_map_;
		mutable std::array<std::string, 32> name_array_;
	};

	// Create a texture from a program.
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- size					: output size (*6).
	std::shared_ptr<Texture> CreateProgramTexture(
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const std::pair<std::uint32_t, std::uint32_t> size,
		const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
		const PixelStructure pixel_structure = PixelStructure::RGB);

	// Create a texture from a program.
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- size					: output size (*6).
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	std::shared_ptr<Texture> CreateProgramTextureMipmap(
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const std::pair<std::uint32_t, std::uint32_t> size,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<sgl::Program> & program)> func = 
				[](const int, const std::shared_ptr<sgl::Program>&) {},
		const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
		const PixelStructure pixel_structure = PixelStructure::RGB);

	// Create a cube map texture from a program.
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- size					: output size (*6).
	std::shared_ptr<TextureCubeMap> CreateProgramTextureCubeMap(
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const std::pair<std::uint32_t, std::uint32_t> size,
		const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
		const PixelStructure pixel_structure = PixelStructure::RGB);

	// Create a cube map texture from a program.
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- size					: output size (*6).
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	std::shared_ptr<TextureCubeMap> CreateProgramTextureCubeMapMipmap(
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const std::pair<std::uint32_t, std::uint32_t> size,
		const int mipmap,
		const std::function<void(
			const int mipmap, 
			const std::shared_ptr<sgl::Program> & program)> func = 
				[](const int, const std::shared_ptr<sgl::Program>&) {},
		const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
		const PixelStructure pixel_structure = PixelStructure::RGB);

} // End namespace sgl.
