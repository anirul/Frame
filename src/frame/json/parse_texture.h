#pragma once

#include <memory>
#include <optional>

#include "frame/json/proto.h"
#include "frame/texture_interface.h"

namespace frame::proto {

	/**
	 * @brief Parse a proto to a texture object.
	 * @param proto_texture: Proto file containing the texture description
	 * @param size: Size of the texture.
	 * @return A unique pointer to an Texture interface.
	 */
	std::unique_ptr<TextureInterface> ParseTexture(
		const proto::Texture& proto_texture,
		glm::uvec2 size);
	/**
	 * @brief Parse a proto to a texture cubemap object.
	 * @param proto_texture: Proto file containing the texture description
	 * @param size: Size of the texture.
	 * @return A unique pointer to an Texture interface.
	 */
	std::unique_ptr<TextureInterface> ParseCubeMapTexture(
		const proto::Texture& proto_texture,
		glm::uvec2 size);
	/**
	 * @brief Parse a texture from a file to a texture object.
	 * @param proto_texture: the input proto.
	 * @return A unique pointer to a texture interface.
	 */
	std::unique_ptr<TextureInterface> ParseTextureFile(
		const proto::Texture& proto_texture);
	/**
	 * @brief Parse a cube map texture from a file.
	 * @param proto_texture: proto for the texture.
	 * @return A unique pointer to a texture interface.
	 */
	std::unique_ptr<TextureInterface> ParseCubeMapTextureFile(
		const proto::Texture& proto_texture);
	/**
	 * @brief Parse a basic texture from a proto and a size.
	 * @param proto_texture: proto for the texture.
	 * @param size: Size of the basic screen (in case you use relative size).
	 * @return A unique pointer to a texture interface.
	 */
	std::unique_ptr<TextureInterface> ParseBasicTexture(
		const proto::Texture& proto_texture,
		glm::uvec2 size);

}  // End namespace frame::proto.
