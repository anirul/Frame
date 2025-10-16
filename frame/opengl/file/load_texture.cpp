#include "frame/opengl/file/load_texture.h"

#include <vector>

#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/serialize_uniform.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/texture.h"

namespace frame::opengl::file
{

std::unique_ptr<TextureInterface> LoadTextureFromVec4(const glm::vec4& vec4)
{
    proto::Texture proto_texture;
    proto_texture.mutable_pixel_element_size()->CopyFrom(
        json::PixelElementSize_FLOAT());
    proto_texture.mutable_pixel_structure()->CopyFrom(
        json::PixelStructure_RGB_ALPHA());
    proto_texture.mutable_size()->CopyFrom(json::SerializeSize({1, 1}));
    proto_texture.set_pixels(
        reinterpret_cast<const char*>(&vec4), sizeof(glm::vec4));
    return std::make_unique<frame::opengl::Texture>(
        proto_texture, glm::uvec2{1, 1});
}

std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f)
{
    proto::Texture proto_texture;
    proto_texture.mutable_pixel_element_size()->CopyFrom(
        json::PixelElementSize_FLOAT());
    proto_texture.mutable_pixel_structure()->CopyFrom(
        json::PixelStructure_GREY());
    proto_texture.mutable_size()->CopyFrom(json::SerializeSize({1, 1}));
    proto_texture.set_pixels(reinterpret_cast<const char*>(&f), sizeof(float));
    return std::make_unique<frame::opengl::Texture>(
        proto_texture, glm::uvec2{1, 1});
}

} // namespace frame::opengl::file
