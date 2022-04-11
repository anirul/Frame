#pragma once

#include <any>
#include <cinttypes>
#include <utility>
#include <glm/glm.hpp>
#include "Frame/BindInterface.h"
#include "Frame/NameInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame {

	// Interface to a texture.
	struct TextureInterface : public BindInterface, public NameInterface
	{
		// Texture filter rename.
		using TextureFilterEnum = frame::proto::TextureFilter::Enum;
		using PixelElementSizeEnum = frame::proto::PixelElementSize::Enum;
		using PixelStructureEnum = frame::proto::PixelStructure::Enum;
		virtual ~TextureInterface() = default;
		// Common interface.
		virtual PixelStructureEnum GetPixelStructure() const = 0;
		virtual PixelElementSizeEnum GetPixelElementSize() const = 0;
		virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
		virtual void EnableMipmap() const = 0;
		virtual void SetMinFilter(const TextureFilterEnum texture_filter) = 0;
		virtual TextureFilterEnum GetMinFilter() const = 0;
		virtual void SetMagFilter(const TextureFilterEnum texture_filter) = 0;
		virtual TextureFilterEnum GetMagFilter() const = 0;
		virtual void SetWrapS(const TextureFilterEnum texture_filter) = 0;
		virtual TextureFilterEnum GetWrapS() const = 0;
		virtual void SetWrapT(const TextureFilterEnum texture_filter) = 0;
		virtual TextureFilterEnum GetWrapT() const = 0;
		virtual void SetWrapR(const TextureFilterEnum texture_filter)
		{
			throw std::runtime_error("Not implemented!");
		}
		virtual TextureFilterEnum GetWrapR() const
		{
			throw std::runtime_error("Not implemented!");
		}
		virtual void Clear(const glm::vec4 color) = 0;
		virtual bool IsCubeMap() const = 0;
		// Get the underlying texture as byte or word or dword.
		// This will be multiply by the pixel structure and multiply by 6 if
		// cubemap.
		virtual std::vector<std::uint8_t> GetTextureByte() const = 0;
		// Can contain either a half or a short.
		virtual std::vector<std::uint16_t> GetTextureWord() const = 0;
		// Can contain either an int or a float.
		virtual std::vector<std::uint32_t> GetTextureDWord() const = 0;
		// Can contain a texture as float.
		virtual std::vector<float> GetTextureFloat() const = 0;
	};

} // End namespace frame.
