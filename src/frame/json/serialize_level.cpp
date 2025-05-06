#include "frame/json/serialize_level.h"

#include "frame/json/serialize_program.h"
#include "frame/json/serialize_texture.h"
#include "frame/logger.h"

namespace frame::proto
{

proto::Level SerializeLevel(LevelInterface& level_interface)
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
        proto::Texture proto_texture = SerializeTexture(texture_interface);
        *proto_level.add_textures() = proto_texture;
    }
    for (const auto& program_id : level_interface.GetPrograms())
    {
        ProgramInterface& program_interface =
            level_interface.GetProgramFromId(program_id);
        proto::Program proto_program =
            SerializeProgram(program_interface, level_interface);
        *proto_level.add_programs() = proto_program;
    }
    return proto_level;
}

} // End namespace frame::proto.
