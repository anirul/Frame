#include "ParseTexture.h"
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/TextureCubeMap.h"
#include "Frame/OpenGL/File/LoadTexture.h"

namespace {
	
	void CheckParameters(const frame::proto::Texture& proto_texture)
	{
		// Get the pixel element size.
		constexpr auto INVALID_ELEMENT_SIZE =
			frame::proto::PixelElementSize::INVALID;
		constexpr auto INVALID_STRUCTURE =
			frame::proto::PixelStructure::INVALID;
		if (proto_texture.pixel_element_size().value() == INVALID_ELEMENT_SIZE)
		{
			throw std::runtime_error("Invalid pixel element size.");
		}
		if (proto_texture.pixel_structure().value() == INVALID_STRUCTURE)
		{
			throw std::runtime_error("Invalid pixel structure.");
		}
	}

} // End namespace.

namespace frame::proto {

	std::unique_ptr<frame::TextureInterface> ParseTexture(
		const Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size)
	{
		CheckParameters(proto_texture);
		std::pair<std::uint32_t, std::uint32_t> texture_size = size;
		if (proto_texture.size().x() < 0)
			texture_size.first /= std::abs(proto_texture.size().x());
		else
			texture_size.first = proto_texture.size().x();
		if (proto_texture.size().y() < 0)
			texture_size.second /= std::abs(proto_texture.size().y());
		else
			texture_size.second = proto_texture.size().y();
		std::unique_ptr<TextureInterface> texture = nullptr;
		if (!proto_texture.pixels().empty())
		{
			texture = std::make_unique<frame::opengl::Texture>(
				texture_size,
				proto_texture.pixels().data(),
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		else
		{
			texture = std::make_unique<frame::opengl::Texture>(
				texture_size,
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		constexpr auto INVALID_TEXTURE = frame::proto::TextureFilter::INVALID;
		if (proto_texture.min_filter().value() != INVALID_TEXTURE)
			texture->SetMinFilter(proto_texture.min_filter().value());
		if (proto_texture.mag_filter().value() != INVALID_TEXTURE)
			texture->SetMagFilter(proto_texture.mag_filter().value());
		if (proto_texture.wrap_s().value() != INVALID_TEXTURE)
			texture->SetWrapS(proto_texture.wrap_s().value());
		if (proto_texture.wrap_t().value() != INVALID_TEXTURE)
			texture->SetWrapT(proto_texture.wrap_t().value());
		return texture;
	}

	std::unique_ptr<TextureInterface> ParseCubeMapTexture(
		const Texture& proto_texture,
		const std::pair<std::uint32_t, std::uint32_t> size)
	{
		// Get the pixel element size.
		CheckParameters(proto_texture);
		std::pair<std::uint32_t, std::uint32_t> texture_size = { 0, 0 };
		if (proto_texture.size().x() < 0)
		{
			texture_size.first /= std::abs(proto_texture.size().x());
		}
		else
		{
			texture_size.first = proto_texture.size().x();
		}
		if (proto_texture.size().y() < 0)
		{
			texture_size.second /= std::abs(proto_texture.size().y());
		}
		else
		{
			texture_size.second = proto_texture.size().y();
		}
		std::unique_ptr<TextureInterface> texture = nullptr;
		if (!proto_texture.pixels().empty())
		{
			throw std::runtime_error("Not implemented!");
		}
		texture = std::make_unique<opengl::TextureCubeMap>(
			texture_size,
			proto_texture.pixel_element_size(),
			proto_texture.pixel_structure());
		constexpr auto INVALID_TEXTURE = frame::proto::TextureFilter::INVALID;
		if (proto_texture.min_filter().value() != INVALID_TEXTURE)
			texture->SetMinFilter(proto_texture.min_filter().value());
		if (proto_texture.mag_filter().value() != INVALID_TEXTURE)
			texture->SetMagFilter(proto_texture.mag_filter().value());
		if (proto_texture.wrap_s().value() != INVALID_TEXTURE)
			texture->SetWrapS(proto_texture.wrap_s().value());
		if (proto_texture.wrap_t().value() != INVALID_TEXTURE)
			texture->SetWrapT(proto_texture.wrap_t().value());
		return texture;
	}

	std::unique_ptr<TextureInterface> ParseTextureFile(
		const proto::Texture& proto_texture)
	{
		CheckParameters(proto_texture);
		return opengl::file::LoadTextureFromFile(
			file::FindFile(proto_texture.file_name()),
			proto_texture.pixel_element_size(),
			proto_texture.pixel_structure());
	}

	std::unique_ptr<TextureInterface> ParseCubeMapTextureFile(
		const proto::Texture& proto_texture)
	{
		CheckParameters(proto_texture);
		if ((proto_texture.file_names().size() != 1) &&
			!proto_texture.file_names().empty())
		{
			if (proto_texture.file_names().size() != 6)
			{
				throw std::runtime_error(
					fmt::format(
						"Invalid file_names size: {}.",
						proto_texture.file_names().size()));
			}
			std::array<std::string, 6> name_array = {};
			for (int i = 0; i < 6; ++i)
			{
				name_array[i] = file::FindFile(proto_texture.file_names()[i]);
			}
			return opengl::file::LoadCubeMapTextureFromFiles(
				name_array,
				proto_texture.pixel_element_size(),
				proto_texture.pixel_structure());
		}
		std::string file_name = (proto_texture.file_name().empty()) ?
			proto_texture.file_names()[0] : proto_texture.file_name();
		return opengl::file::LoadCubeMapTextureFromFile(
			file_name,
			proto_texture.pixel_element_size(),
			proto_texture.pixel_structure());
	}

} // End namespace frame::proto.
