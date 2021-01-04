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
#include "Frame/Error.h"
#include "Frame/OpenGL/Pixel.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/OpenGL/ScopedBind.h"
#include "Frame/Proto/Proto.h"

namespace frame::opengl {

	class Texture : public TextureInterface
	{
	public:
		// Create an empty texture of size size.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create a texture from a file.
		Texture(
			const std::string& file,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a raw pointer.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a proto.
		Texture(
			const frame::proto::Texture& proto_texture, 
			const std::pair<std::uint32_t, std::uint32_t> size);
		virtual ~Texture();

	public:
		// Texture filter rename.
		using TextureFilter = frame::proto::Texture::TextureFilterEnum;
		// Convert to GL type and from GL type.
		int ConvertToGLType(TextureFilter texture_filter) const;
		TextureFilter ConvertFromGLType(int gl_filter) const;
		// Bind and unbind a texture to the current context.
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		// Clear the texture.
		void Clear(const glm::vec4 color);
		// Virtual part.
		virtual void BindEnableMipmap() const;
		virtual void SetMinFilter(const TextureFilter texture_filter);
		virtual TextureFilter GetMinFilter() const;
		virtual void SetMagFilter(const TextureFilter texture_filter);
		virtual TextureFilter GetMagFilter() const;
		virtual void SetWrapS(const TextureFilter texture_filter);
		virtual TextureFilter GetWrapS() const;
		virtual void SetWrapT(const TextureFilter texture_filter);
		virtual TextureFilter GetWrapT() const;

	public:
		const unsigned int GetId() const override { return texture_id_; }
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
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB()) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }
		friend class ScopedBind;

	protected:
		unsigned int texture_id_ = 0;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_;
		const PixelStructure pixel_structure_;
		const Error& error_ = Error::GetInstance();
		mutable bool locked_bind_ = false;
		std::shared_ptr<RenderBuffer> render_ = nullptr;
		std::shared_ptr<FrameBuffer> frame_ = nullptr;
	};

	class TextureCubeMap : public Texture
	{
	public:
		// Create an empty cube map of the size size.
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Take a single texture and map it to a cube view.
		TextureCubeMap(
			const std::string& file_name,
			const std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 },
			const PixelElementSize& pixel_element_size =
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Take 6 texture to be mapped to the cube map, Order is:
		// right, left - (positive X, negative X)
		// top, bottom - (positive Y, negative Y)
		// front, back - (positive Z, negative Z)
		// The size is equal to the size of an image (*6).
		TextureCubeMap(
			const std::array<std::string, 6>& cube_file,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a proto.
		// the size is the preferred size of the screen.
		TextureCubeMap(
			const frame::proto::Texture& texture,
			const std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Inside constructors to be called from proto and normal constructor.
		void InitCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size,
			const PixelStructure& pixel_structure);
		void InitCubeMapFromSixFiles(
			const std::array<std::string, 6> file_names,
			const PixelElementSize pixel_element_size,
			const PixelStructure pixel_structure);
		void InitCubeMapFromFile(
			const std::string& file_name,
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size,
			const PixelStructure& pixel_structure);

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		void BindEnableMipmap() const override;
		void SetMinFilter(const TextureFilter texture_filter) override;
		TextureFilter GetMinFilter() const override;
		void SetMagFilter(const TextureFilter texture_filter) override;
		TextureFilter GetMagFilter() const override;
		void SetWrapS(const TextureFilter texture_filter) override;
		TextureFilter GetWrapS() const override;
		void SetWrapT(const TextureFilter texture_filter) override;
		TextureFilter GetWrapT() const override;
		void SetWrapR(const TextureFilter texture_filter);
		TextureFilter GetWrapR() const;

	protected:
		// Create a cube map and assign it to the texture_id_.
		void CreateTextureCubeMap();
	};

	// This should be in Resource.
	std::shared_ptr<Texture> LoadTextureFromFile(
		std::istream& is,
		const std::string& stream_name,
		const std::string& element_name);
	std::shared_ptr<Texture> LoadTextureFrom3Float(
		std::istream& is,
		const std::string& stream_name,
		const std::string& element_name);
	std::shared_ptr<Texture> LoadTextureFrom1Float(
		std::istream& is,
		const std::string& stream_name,
		const std::string& element_name);

} // End namespace frame::opengl.
