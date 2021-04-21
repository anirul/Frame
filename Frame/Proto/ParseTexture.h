#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/TextureInterface.h"

namespace frame::proto {

	std::shared_ptr<TextureInterface> ParseTexture(
		const proto::Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size);

	std::shared_ptr<TextureInterface> ParseCubeMapTexture(
		const proto::Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size);

	std::shared_ptr<TextureInterface> ParseTextureFile(
		const proto::Texture& proto_texture);

	std::shared_ptr<TextureInterface> ParseCubeMapTextureFile(
		const proto::Texture& proto_texture);

} // End namespace frame::proto.
