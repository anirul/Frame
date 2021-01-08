#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/TextureInterface.h"

namespace frame::proto {

	std::shared_ptr<TextureInterface> ParseTexture(
		const frame::proto::Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size);

	std::shared_ptr<TextureInterface> ParseCubeMapTexture(
		const frame::proto::Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size);

} // End namespace frame::proto.
