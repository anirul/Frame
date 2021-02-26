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
#include "Frame/Proto/ParsePixel.h"
#include "Frame/Proto/Proto.h"
#include "Frame/TextureInterface.h"

namespace frame::opengl {

	class Texture : 
		public TextureInterface, 
		public std::enable_shared_from_this<Texture>
	{
	public:
		// Create an empty texture of size size.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const proto::PixelElementSize& pixel_element_size = 
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB());
		// Create from a raw pointer.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const proto::PixelElementSize& pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB());
		// Create from a proto.
		virtual ~Texture();

	public:
		// Convert to GL type and from GL type.
		int ConvertToGLType(TextureFilterEnum texture_filter) const;
		TextureFilterEnum ConvertFromGLType(int gl_filter) const;
		// Bind and unbind a texture to the current context.
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		// Clear the texture.
		void Clear(const glm::vec4 color) override;
		// Virtual part.
		virtual void EnableMipmap() const override;
		virtual void SetMinFilter(
			const TextureFilterEnum texture_filter) override;
		virtual TextureFilterEnum GetMinFilter() const override;
		virtual void SetMagFilter(
			const TextureFilterEnum texture_filter) override;
		virtual TextureFilterEnum GetMagFilter() const override;
		virtual void SetWrapS(
			const TextureFilterEnum texture_filter) override;
		virtual TextureFilterEnum GetWrapS() const override;
		virtual void SetWrapT(const TextureFilterEnum texture_filter) override;
		virtual TextureFilterEnum GetWrapT() const override;
		virtual bool IsCubeMap() const override { return false; }

	public:
		unsigned int GetId() const override { return texture_id_; }
		std::pair<std::uint32_t, std::uint32_t> GetSize() const override
		{ 
			return size_; 
		}
		const frame::proto::PixelElementSize GetPixelElementSize() const
		{
			return pixel_element_size_; 
		}
		const frame::proto::PixelStructure GetPixelStructure() const
		{
			return pixel_structure_;
		}

	protected:
		void CreateTexture();
		Texture(
			const proto::PixelElementSize& pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB()) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }
		friend class ScopedBind;

	protected:
		unsigned int texture_id_ = 0;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const proto::PixelElementSize pixel_element_size_;
		const proto::PixelStructure pixel_structure_;
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
			const proto::PixelElementSize& pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB());
		// Create from a ray pointer.
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const proto::PixelElementSize& pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB());
		// Create from 6 pointer to be mapped to the cube map, Order is:
		// right, left - (positive X, negative X)
		// top, bottom - (positive Y, negative Y)
		// front, back - (positive Z, negative Z)
		// The size is equal to the size of an image (*6).
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const std::array<void*, 6> cube_data,
			const proto::PixelElementSize& pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure& pixel_structure = 
				proto::PixelStructure_RGB());

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		void EnableMipmap() const override;
		void SetMinFilter(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetMinFilter() const override;
		void SetMagFilter(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetMagFilter() const override;
		void SetWrapS(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetWrapS() const override;
		void SetWrapT(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetWrapT() const override;
		void SetWrapR(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetWrapR() const override;

	public:
		bool IsCubeMap() const override { return true; }

	protected:
		// Create a cube map and assign it to the texture_id_.
		void CreateTextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const std::array<void*, 6> cube_map = 
				{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr });
	};

} // End namespace frame::opengl.
