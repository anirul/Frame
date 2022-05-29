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

#include "Frame/OpenGL/Pixel.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/OpenGL/ScopedBind.h"
#include "Frame/Proto/ParsePixel.h"
#include "Frame/Proto/Proto.h"
#include "Frame/TextureInterface.h"

namespace frame::opengl {

	class Texture : public TextureInterface
	{
	public:
		// Create an empty texture of size size.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const proto::PixelElementSize pixel_element_size = 
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure = 
				proto::PixelStructure_RGB());
		// Create from a raw pointer.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const proto::PixelElementSize pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure = 
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
		// Get a copy of the texture output.
        std::vector<std::uint8_t> GetTextureByte() const override;
        std::vector<std::uint16_t> GetTextureWord() const override;
        std::vector<std::uint32_t> GetTextureDWord() const override;
		std::vector<float> GetTextureFloat() const override;
		// Clear the texture.
		void Clear(const glm::vec4 color) override;
		// Name interface.
		std::string GetName() const override { return name_; }
		void SetName(const std::string& name) override { name_ = name; }

	public:
		// Virtual part.
		void EnableMipmap() const override;
		void SetMinFilter(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetMinFilter() const override;
		void SetMagFilter(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetMagFilter() const override;
		void SetWrapS(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetWrapS() const override;
		void SetWrapT(const TextureFilterEnum texture_filter) override;
		TextureFilterEnum GetWrapT() const override;

	public:
		bool IsCubeMap() const final { return false; }
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
		Texture(
			const proto::PixelElementSize pixel_element_size =
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
				proto::PixelStructure_RGB()) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}
		void CreateTexture(const void* data = nullptr);
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
		mutable bool locked_bind_ = false;
		std::unique_ptr<RenderBuffer> render_ = nullptr;
		std::unique_ptr<FrameBuffer> frame_ = nullptr;
		std::string name_;
	};

} // End namespace frame::opengl.
