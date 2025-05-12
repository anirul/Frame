#include "frame/json/serialize_texture.h"

#include "frame/opengl/cubemap.h"
#include "frame/json/serialize_uniform.h"

namespace frame::json
{

namespace
{

proto::Texture SerializeTextureFromParameter(
    const TextureParameter& texture_parameter)
{
    proto::Texture proto_texture;
    *proto_texture.mutable_pixel_element_size() =
        texture_parameter.pixel_element_size;
    *proto_texture.mutable_pixel_structure() =
        texture_parameter.pixel_structure;
    *proto_texture.mutable_size() = SerializeSize(texture_parameter.size);
    return proto_texture;
}

} // End anonymous namespace.

proto::Texture SerializeTexture(TextureInterface& texture_interface)
{
    TextureParameter texture_parameter =
        texture_interface.GetTextureParameter();
    proto::Texture proto_texture =
        SerializeTextureFromParameter(texture_interface.GetTextureParameter());
    proto_texture.set_name(texture_interface.GetName());
    proto_texture.set_cubemap(texture_interface.IsCubeMap());
    // TODO(anirul): Get the file names?
    return proto_texture;
}

} // End namespace frame::json.
