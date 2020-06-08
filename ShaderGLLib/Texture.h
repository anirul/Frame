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

	enum class TextureFilter 
	{
		NEAREST = GL_NEAREST,
		LINEAR = GL_LINEAR,
		NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR,
		CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
		MIRROR_REPEAT = GL_MIRRORED_REPEAT,
		REPEAT = GL_REPEAT
	};

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
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		virtual ~Texture();

	public:
		virtual void Bind(const unsigned int slot = 0) const;
		virtual void UnBind() const;
		virtual void BindEnableMipmap() const;
		virtual void SetMinFilter(TextureFilter texture_filter);
		virtual TextureFilter GetMinFilter() const;
		virtual void SetMagFilter(TextureFilter texture_filter);
		virtual TextureFilter GetMagFilter() const;
		virtual void SetWrapS(TextureFilter texture_filter);
		virtual TextureFilter GetWrapS() const;
		virtual void SetWrapT(TextureFilter texture_filter);
		virtual TextureFilter GetWrapT() const;

	public:
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
		void CreateTexture();
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
		// Create an empty cube map of the size size.
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
		void SetMinFilter(TextureFilter texture_filter) override;
		TextureFilter GetMinFilter() const override;
		void SetMagFilter(TextureFilter texture_filter) override;
		TextureFilter GetMagFilter() const override;
		void SetWrapS(TextureFilter texture_filter) override;
		TextureFilter GetWrapS() const override;
		void SetWrapT(TextureFilter texture_filter) override;
		TextureFilter GetWrapT() const override;
		void SetWrapR(TextureFilter texture_filter);
		TextureFilter GetWrapR() const;

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
		bool HasTexture(const std::string& name) const;
		bool RemoveTexture(const std::string& name);
		// Return the binding slot of the texture (to be passed to the program).
		const int EnableTexture(const std::string& name) const;
		const std::vector<std::string> GetTexturesNames() const;
		void DisableTexture(const std::string& name) const;
		void DisableAll() const;

	private:
		std::map<std::string, std::shared_ptr<Texture>> name_texture_map_;
		mutable std::array<std::string, 32> name_array_;
	};

	// Get the brightness from a texture (usually before HDR).
	std::shared_ptr<Texture> TextureBrightness(
		const std::shared_ptr<Texture>& texture);

	// Add blur to a texture.
	std::shared_ptr<Texture> TextureBlur(
		const std::shared_ptr<Texture>& in_texture,
		const float exponent = 1.0f);

	// Get the Gaussian blur of a texture.
	std::shared_ptr<Texture> TextureGaussianBlur(
		const std::shared_ptr<Texture>& texture);

	// Vector addition a number of texture (maximum 16) into one.
	std::shared_ptr<Texture> TextureAddition(
		const std::vector<std::shared_ptr<Texture>>& add_textures);

	// Vector multiply a number of texture (maximum 16) into one.
	std::shared_ptr<Texture> TextureMultiply(
		const std::vector<std::shared_ptr<Texture>>& multiply_textures);

	// Fill multiple textures from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	void FillProgramMultiTexture(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program);

	// Fill multiple textures from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<sgl::Program>& program)> func =
		[](const int, const std::shared_ptr<sgl::Program>&) {});

	// Fill multiple cube map texture from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	void FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program);

	// Fill multiple cube map texture from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- texture_manager		: input texture.
	//		- texture_selected		: set of selected texture to be used.
	//		- program				: program to be used.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const TextureManager& texture_manager,
		const std::vector<std::string>& texture_selected,
		const std::shared_ptr<Program>& program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<Program>& program)> func =
				[](const int, const std::shared_ptr<Program>&) {});

} // End namespace sgl.
