#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <map>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	class Texture 
	{
	public:
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
		const int GetId() const { return texture_id_; }
		std::pair<size_t, size_t> GetSize() const { return size_; }
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
		std::pair<size_t, size_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_;
		const PixelStructure pixel_structure_;
		const std::shared_ptr<Error> error_ = Error::GetInstance();
	};

	class TextureCubeMap : public Texture
	{
	public:
		// Take 6 texture to be mapped to the cube map, Order is:
		// right, left - (positive X, negative X)
		// top, bottom - (positive Y, negative Y)
		// front, back - (positive Z, negative Z)
		TextureCubeMap(
			const std::array<std::string, 6>& cube_file,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);
		// Take a single texture and map it to a cube view.
		TextureCubeMap(
			const std::string& file_name,
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB);

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		void BindEnableMipmap() const override;

	protected:
		// Create a cube map and assign it to the texture_id_.
		void CreateCubeMap();

	protected:
		// Get the 6 view for the cube map.
		const std::array<glm::mat4, 6> views_ = 
		{
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(1.0f,  0.0f,  0.0f),
				glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(-1.0f,  0.0f,  0.0f),
				glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(0.0f,  1.0f,  0.0f),
				glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(0.0f, -1.0f,  0.0f),
				glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(0.0f,  0.0f,  1.0f),
				glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(
				glm::vec3(0.0f,  0.0f,  0.0f),
				glm::vec3(0.0f,  0.0f, -1.0f),
				glm::vec3(0.0f, -1.0f,  0.0f))
		};
	};

	class TextureManager 
	{
	public:
		TextureManager() = default;
		virtual ~TextureManager();

	public:
		bool AddTexture(
			const std::string& name, 
			const std::shared_ptr<sgl::Texture>& texture);
		bool RemoveTexture(const std::string& name);
		// Return the binding slot of the texture (to be passed to the program).
		const int EnableTexture(const std::string& name) const;
		void DisableTexture(const std::string& name) const;
		void DisableAll() const;

	private:
		std::map<std::string, std::shared_ptr<Texture>> name_texture_map_;
		mutable std::array<std::string, 32> name_array_;
	};

} // End namespace sgl.
