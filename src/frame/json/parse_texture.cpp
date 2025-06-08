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
    CheckParameters(proto_texture);
    if (proto_texture.cubemap())
    {
        return ParseCubemap(proto_texture, size);
    }
    std::unique_ptr<TextureInterface> texture_interface =
        std::make_unique<opengl::Texture>(proto_texture, size);
    return texture_interface;
}

std::unique_ptr<TextureInterface> ParseCubemap(
    const proto::Texture& proto_texture, glm::uvec2 size)
{
    CheckParameters(proto_texture);
    if (!proto_texture.cubemap())
    {
        return ParseTexture(proto_texture, size);
    }
    std::unique_ptr<TextureInterface> texture_interface =
        std::make_unique<opengl::Cubemap>(proto_texture, size);
    return texture_interface;
}

} // End namespace frame::json.
