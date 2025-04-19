#include "frame/json/serialize_level.h"

#include "frame/json/serialize_texture.h"
#include "frame/logger.h"

namespace frame::proto
{

proto::Level SerializeLevel(LevelInterface& level)
{
    proto::Level proto;
    auto logger = Logger::GetInstance();
    proto.set_name(level.GetName());
    proto.set_default_texture_name(
        level.GetTextureFromId(level.GetDefaultOutputTextureId()).GetName());
    for (const auto& texture_id : level.GetTextures())
    {
        TextureInterface& texture_interface =
            level.GetTextureFromId(texture_id);
        proto::Texture proto_texture =
            SerializeTexture(texture_interface);
        *proto.add_textures() = proto_texture;
    }
    return proto;
}

} // End namespace frame::proto.
