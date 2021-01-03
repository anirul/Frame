#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "../Frame/TextureInterface.h"

namespace frame {

	struct MaterialInterface 
	{
		// CHECKME(anirul): is this really needed?
		// Texture management part.
		virtual bool AddTextureId(std::uint64_t id) = 0;
		virtual bool HasTextureId(std::uint64_t id) const = 0;
		virtual bool RemoveTextureId(std::uint64_t id) = 0;
		// Return the binding slot of a texture (to be passed to the program).
		virtual const int EnableTexture(
			const std::shared_ptr<TextureInterface> texture) const = 0;
		// Unbind the texture and remove it from the list.
		virtual void DisableTexture(
			const std::shared_ptr<TextureInterface> texture) const = 0;
		// Disable all the texture and unbind them.
		virtual void DisableAll(
			const std::vector<std::shared_ptr<TextureInterface>> textures) 
			const = 0;
	};

} // End namespace frame.
