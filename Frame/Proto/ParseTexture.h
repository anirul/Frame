#pragma once

#include <memory>
#include <optional>
#include "Frame/Proto/Proto.h"
#include "Frame/TextureInterface.h"

namespace frame::proto {

	std::optional<std::unique_ptr<TextureInterface>> 
		ParseTexture(
			const proto::Texture& proto_texture,
			const std::pair<std::uint32_t, std::uint32_t> size);

	std::optional<std::unique_ptr<TextureInterface>> 
		ParseCubeMapTexture(
			const proto::Texture& proto_texture,
			const std::pair<std::uint32_t, std::uint32_t> size);

	std::optional<std::unique_ptr<TextureInterface>> 
		ParseTextureFile(
			const proto::Texture& proto_texture);

	std::optional<std::unique_ptr<TextureInterface>> 
		ParseCubeMapTextureFile(
			const proto::Texture& proto_texture);

} // End namespace frame::proto.
