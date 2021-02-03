#include "ParseTexture.h"
#include "Frame/Error.h"
#include "Frame/OpenGL/Texture.h"

namespace frame::proto {

	std::shared_ptr<frame::TextureInterface> ParseTexture(
		const Texture& proto_texture, 
		const std::pair<std::uint32_t, std::uint32_t> size)
	{
		auto& error = Error::GetInstance();
		// Get the pixel element size.
		constexpr auto INVALID_ELEMENT_SIZE =
			frame::proto::PixelElementSize::INVALID;
		constexpr auto INVALID_STRUCTURE =
			frame::proto::PixelStructure::INVALID;
		if (proto_texture.pixel_element_size().value() == INVALID_ELEMENT_SIZE)
		{
			error.CreateError(
				"Invalid pixel element size.",
				__FILE__,
				__LINE__ - 7);
		}
		if (proto_texture.pixel_structure().value() == INVALID_STRUCTURE)
		{
			error.CreateError(
				"Invalid pixel structure.",
				__FILE__,
				__LINE__ - 7);
		}
		std::pair<std::uint32_t, std::uint32_t> texture_size = { 0, 0 };
		if (proto_texture.size().x() < 0)
			texture_size.first /= std::abs(proto_texture.size().x());
		else
			texture_size.first = proto_texture.size().x();
		if (proto_texture.size().y() < 0)
			texture_size.second /= std::abs(proto_texture.size().y());
		else
			texture_size.second = proto_texture.size().y();
		std::shared_ptr<TextureInterface> texture = nullptr;
		if (proto_texture.pixels().size())
		{
			texture = std::make_shared<frame::opengl::Texture>(
				texture_size,
				proto_texture.pixels().data(),
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		else
		{
			texture = std::make_shared<frame::opengl::Texture>(
				texture_size,
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		constexpr auto INVALID_TEXTURE = frame::proto::Texture::INVALID;
		if (proto_texture.min_filter() != INVALID_TEXTURE)
			texture->SetMinFilter(proto_texture.min_filter());
		if (proto_texture.mag_filter() != INVALID_TEXTURE)
			texture->SetMagFilter(proto_texture.mag_filter());
		if (proto_texture.wrap_s() != INVALID_TEXTURE)
			texture->SetWrapS(proto_texture.wrap_s());
		if (proto_texture.wrap_t() != INVALID_TEXTURE)
			texture->SetWrapT(proto_texture.wrap_t());
		return texture;
	}

	std::shared_ptr<frame::TextureInterface> ParseCubeMapTexture(
		const Texture& proto_texture, 
		const std::pair<std::uint32_t, std::uint32_t> size)
	{
		auto& error = Error::GetInstance();
		// Get the pixel element size.
		constexpr auto INVALID_ELEMENT_SIZE =
			frame::proto::PixelElementSize::INVALID;
		constexpr auto INVALID_STRUCTURE =
			frame::proto::PixelStructure::INVALID;
		if (proto_texture.pixel_element_size().value() == INVALID_ELEMENT_SIZE)
		{
			error.CreateError(
				"Invalid pixel element size.",
				__FILE__,
				__LINE__ - 7);
		}
		if (proto_texture.pixel_structure().value() == INVALID_STRUCTURE)
		{
			error.CreateError(
				"Invalid pixel structure.",
				__FILE__,
				__LINE__ - 7);
		}
		std::pair<std::uint32_t, std::uint32_t> texture_size = { 0, 0 };
		if (proto_texture.size().x() < 0)
			texture_size.first /= std::abs(proto_texture.size().x());
		else
			texture_size.first = proto_texture.size().x();
		if (proto_texture.size().y() < 0)
			texture_size.second /= std::abs(proto_texture.size().y());
		else
			texture_size.second = proto_texture.size().y();
		std::shared_ptr<TextureInterface> texture = nullptr;
		if (proto_texture.pixels().size())
		{
			texture = std::make_shared<opengl::TextureCubeMap>(
				texture_size,
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		else
		{
			texture = std::make_shared<opengl::TextureCubeMap>(
				texture_size,
				proto_texture.pixels().data(),
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		constexpr auto INVALID_TEXTURE = frame::proto::Texture::INVALID;
		if (proto_texture.min_filter() != INVALID_TEXTURE)
			texture->SetMinFilter(proto_texture.min_filter());
		if (proto_texture.mag_filter() != INVALID_TEXTURE)
			texture->SetMagFilter(proto_texture.mag_filter());
		if (proto_texture.wrap_s() != INVALID_TEXTURE)
			texture->SetWrapS(proto_texture.wrap_s());
		if (proto_texture.wrap_t() != INVALID_TEXTURE)
			texture->SetWrapT(proto_texture.wrap_t());
		if (proto_texture.wrap_r() != INVALID_TEXTURE)
			texture->SetWrapR(proto_texture.wrap_r());
		return texture;
	}

} // End namespace frame::proto.
