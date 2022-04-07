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

	proto::TextureFrame GetTextureFrameFromPosition(int i);

	class TextureCubeMap : public TextureInterface
	{
	public:
		// Create an empty cube map of the size size.
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const proto::PixelElementSize pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
				proto::PixelStructure_RGB());
		// Create from 6 pointer to be mapped to the cube map, Order is:
		//     right, left - (positive X, negative X)
		//     top, bottom - (positive Y, negative Y)
		//     front, back - (positive Z, negative Z)
		// The size is equal to the size of an image (*6).
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const std::array<void*, 6> cube_data,
			const proto::PixelElementSize pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
				proto::PixelStructure_RGB());
		~TextureCubeMap();

	public:
		// Convert to GL type and from GL type.
		int ConvertToGLType(TextureFilterEnum texture_filter) const;
		TextureFilterEnum ConvertFromGLType(int gl_filter) const;
		// Get a copy of the texture output, i is the texture number in case of
		// a cube map (check IsCubeMap).
		std::pair<void*, std::size_t> GetTexture(int i) const override;
		// Clear the texture.
		void Clear(const glm::vec4 color) override;
		// Name interface.
		std::string GetName() const override { return name_; }
		void SetName(const std::string& name) override { name_ = name; }

	public:
		// Bind and unbind a texture to the current context.
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
		bool IsCubeMap() const final { return true; }
		unsigned int GetId() const override { return texture_id_; }
		std::pair<std::uint32_t, std::uint32_t> GetSize() const override
		{
			return size_;
		}
		proto::PixelElementSize::Enum GetPixelElementSize() const override
		{
			return pixel_element_size_.value();
		}
		proto::PixelStructure::Enum GetPixelStructure() const override
		{
			return pixel_structure_.value();
		}

	protected:
		TextureCubeMap(
			const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB()) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}
		// Create a cube map and assign it to the texture_id_.
		void CreateTextureCubeMap(
			const std::array<void*, 6> cube_map =
			{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr });
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }

	protected:
		void CreateFrameAndRenderBuffer();
		friend class ScopedBind;

	private:
		unsigned int texture_id_ = 0;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const proto::PixelElementSize pixel_element_size_;
		const proto::PixelStructure pixel_structure_;
		const Error& error_ = Error::GetInstance();
		mutable bool locked_bind_ = false;
		std::unique_ptr<RenderBuffer> render_ = nullptr;
		std::unique_ptr<FrameBuffer> frame_ = nullptr;
		std::string name_ = "";
	};

} // End namespace frame::opengl.
