#include "frame/json/parse_texture.h"

#include <filesystem>

#include "frame/file/file_system.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/texture.h"

namespace
{

void CheckParameters(const frame::proto::Texture& proto_texture)
{
    // Get the pixel element size.
    constexpr auto INVALID_ELEMENT_SIZE =
        frame::proto::PixelElementSize::INVALID;
    constexpr auto INVALID_STRUCTURE = frame::proto::PixelStructure::INVALID;
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

namespace frame::json
{

std::unique_ptr<frame::TextureInterface> ParseTexture(
    const proto::Texture& proto_texture, glm::uvec2 size)
{
    glm::uvec2 texture_size = size;
    glm::uvec2 relative_texture_size = glm::uvec2(0, 0);
    if (proto_texture.size().x() < 0)
    {
        relative_texture_size.x = proto_texture.size().x();
        texture_size.x /= std::abs(proto_texture.size().x());
    }
    else
    {
        texture_size.x = proto_texture.size().x();
    }
    if (proto_texture.size().y() < 0)
    {
        relative_texture_size.y = proto_texture.size().y();
        texture_size.y /= std::abs(proto_texture.size().y());
    }
    else
    {
        texture_size.y = proto_texture.size().y();
    }
    std::unique_ptr<TextureInterface> texture = nullptr;
    TextureParameter texture_parameter = {};
    texture_parameter.pixel_element_size = proto_texture.pixel_element_size();
    texture_parameter.pixel_structure = proto_texture.pixel_structure();
    texture_parameter.size = texture_size;
    texture_parameter.relative_size = relative_texture_size;
    if (!proto_texture.pixels().empty())
    {
        texture_parameter.data_ptr = (void*)proto_texture.pixels().data();
        texture = std::make_unique<frame::opengl::Texture>(texture_parameter);
    }
    else
    {
        texture = std::make_unique<frame::opengl::Texture>(texture_parameter);
    }
    constexpr auto INVALID_TEXTURE = frame::proto::TextureFilter::INVALID;
    if (proto_texture.min_filter().value() != INVALID_TEXTURE)
    {
        texture->SetMinFilter(proto_texture.min_filter().value());
    }
    if (proto_texture.mag_filter().value() != INVALID_TEXTURE)
    {
        texture->SetMagFilter(proto_texture.mag_filter().value());
    }
    if (proto_texture.wrap_s().value() != INVALID_TEXTURE)
    {
        texture->SetWrapS(proto_texture.wrap_s().value());
    }
    if (proto_texture.wrap_t().value() != INVALID_TEXTURE)
    {
        texture->SetWrapT(proto_texture.wrap_t().value());
    }
    return texture;
}

std::unique_ptr<TextureInterface> ParseCubeMapTexture(
    const proto::Texture& proto_texture, glm::uvec2 size)
{
    glm::uvec2 texture_size = size;
    glm::uvec2 relative_texture_size = size;
    if (proto_texture.size().x() < 0)
    {
        relative_texture_size.x = proto_texture.size().x();
        texture_size.x /= std::abs(proto_texture.size().x());
    }
    else
    {
        texture_size.x = proto_texture.size().x();
    }
    if (proto_texture.size().y() < 0)
    {
        relative_texture_size.y = proto_texture.size().y();
        texture_size.y /= std::abs(proto_texture.size().y());
    }
    else
    {
        texture_size.y = proto_texture.size().y();
    }
    std::unique_ptr<TextureInterface> texture = nullptr;
    if (!proto_texture.pixels().empty())
    {
        throw std::runtime_error("Not implemented!");
    }
    TextureParameter texture_parameter = {};
    texture_parameter.pixel_element_size = proto_texture.pixel_element_size();
    texture_parameter.pixel_structure = proto_texture.pixel_structure();
    texture_parameter.map_type = TextureTypeEnum::CUBMAP;
    texture_parameter.size = texture_size;
    texture_parameter.relative_size = relative_texture_size;
    texture = std::make_unique<opengl::Cubemap>(texture_parameter);
    constexpr auto INVALID_TEXTURE = frame::proto::TextureFilter::INVALID;
    if (proto_texture.min_filter().value() != INVALID_TEXTURE)
    {
        texture->SetMinFilter(proto_texture.min_filter().value());
    }
    if (proto_texture.mag_filter().value() != INVALID_TEXTURE)
    {
        texture->SetMagFilter(proto_texture.mag_filter().value());
    }
    if (proto_texture.wrap_s().value() != INVALID_TEXTURE)
    {
        texture->SetWrapS(proto_texture.wrap_s().value());
    }
    if (proto_texture.wrap_t().value() != INVALID_TEXTURE)
    {
        texture->SetWrapT(proto_texture.wrap_t().value());
    }
    return texture;
}

std::unique_ptr<TextureInterface> ParseTextureFile(
    const proto::Texture& proto_texture)
{
    return opengl::file::LoadTextureFromFile(
        file::FindFile(std::filesystem::path(proto_texture.file_name())),
        proto_texture.pixel_element_size(),
        proto_texture.pixel_structure());
}

std::unique_ptr<TextureInterface> ParseCubeMapTextureFile(
    const proto::Texture& proto_texture)
{
    return opengl::file::LoadCubeMapTextureFromFile(
        file::FindFile(std::filesystem::path(proto_texture.file_name())),
        proto_texture.pixel_element_size(),
        proto_texture.pixel_structure());
}

std::unique_ptr<TextureInterface> ParseCubeMapTextureFiles(
    const proto::Texture& proto_texture)
{
    std::array<std::filesystem::path, 6> name_array = {
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().positive_x())),
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().negative_x())),
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().positive_y())),
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().negative_y())),
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().positive_z())),
        file::FindFile(
            std::filesystem::path(proto_texture.file_names().negative_z()))};
    return opengl::file::LoadCubeMapTextureFromFiles(
        name_array,
        proto_texture.pixel_element_size(),
        proto_texture.pixel_structure());
}

std::unique_ptr<frame::TextureInterface> ParseBasicTexture(
    const proto::Texture& proto_texture, glm::uvec2 size)
{
    CheckParameters(proto_texture);
    if (proto_texture.has_file_name() && !proto_texture.cubemap())
    {
        return ParseTextureFile(proto_texture);
    }
    if (proto_texture.has_file_name() && proto_texture.cubemap())
    {
        return ParseCubeMapTextureFile(proto_texture);
    }
    if (proto_texture.has_file_names() && proto_texture.cubemap())
    {
        return ParseCubeMapTextureFiles(proto_texture);
    }
    if (proto_texture.cubemap())
    {
        return ParseCubeMapTexture(proto_texture, size);
    }
    return ParseTexture(proto_texture, size);
}

} // End namespace frame::json.
