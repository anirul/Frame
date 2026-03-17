#include "frame/json/serialize_level.h"

#include <array>

#include "frame/json/serialize_scene_tree.h"
#include "frame/json/serialize_texture.h"
#include "frame/logger.h"

namespace frame::json
{

proto::Level SerializeLevel(const LevelInterface& level_interface)
{
    proto::Level proto_level;
    auto logger = Logger::GetInstance();
    proto_level.set_name(level_interface.GetName());
    proto_level.set_default_texture_name(
        level_interface
            .GetTextureFromId(level_interface.GetDefaultOutputTextureId())
            .GetName());
    for (const auto& texture_id : level_interface.GetTextures())
    {
        TextureInterface& texture_interface =
            level_interface.GetTextureFromId(texture_id);
        if (!texture_interface.SerializeEnable())
        {
            continue;
        }
        proto::Texture proto_texture = SerializeTexture(texture_interface);
        *proto_level.add_textures() = proto_texture;
    }
    *proto_level.mutable_scene_tree() = SerializeSceneTree(level_interface);
    return proto_level;
}

} // End namespace frame::json.
