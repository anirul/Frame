#include "frame/json/serialize_texture.h"

#include "frame/opengl/cubemap.h"

namespace frame::proto
{

proto::Texture SerializeTexture(TextureInterface& texture_interface)
{
    proto::Texture proto_texture;
    proto_texture.set_name(texture_interface.GetName());
    proto_texture.set_mipmap(texture_interface.IsCubeMap());
    TextureParameter texture_parameter =
        texture_interface.GetTextureParameter();
    
    return proto_texture;
}

} // End namespace frame::proto.
