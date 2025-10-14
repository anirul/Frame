#include "frame/vulkan/json/parse_texture.h"

#include <stdexcept>

#include "frame/vulkan/texture.h"

namespace frame::vulkan::json
{

namespace
{

void ValidateTexture(const frame::proto::Texture& proto_texture)
{
    if (proto_texture.pixel_element_size().value() ==
        frame::proto::PixelElementSize::INVALID)
    {
        throw std::runtime_error("Invalid pixel element size for texture.");
    }
    if (proto_texture.pixel_structure().value() ==
        frame::proto::PixelStructure::INVALID)
    {
        throw std::runtime_error("Invalid pixel structure for texture.");
    }
    if (proto_texture.cubemap())
    {
        throw std::runtime_error("Cubemap textures are not supported in Vulkan yet.");
    }
}

} // namespace

std::unique_ptr<frame::TextureInterface> ParseTexture(
    const frame::proto::Texture& proto_texture, glm::uvec2 size)
{
    ValidateTexture(proto_texture);
    auto texture = std::make_unique<frame::vulkan::Texture>(
        proto_texture, size);
    texture->SetSerializeEnable(true);
    return texture;
}

} // namespace frame::vulkan::json
