#include "frame/json/serialize_level.h"

#include <array>

#include "frame/json/serialize_material.h"
#include "frame/json/serialize_program.h"
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
    for (const auto& material_id : level_interface.GetMaterials())
    {
        MaterialInterface& material_interface =
            level_interface.GetMaterialFromId(material_id);
        if (!material_interface.SerializeEnable())
        {
            continue;
        }
        proto::Material proto_material =
            SerializeMaterial(material_interface, level_interface);
        *proto_level.add_materials() = proto_material;
    }
    for (const auto& program_id : level_interface.GetPrograms())
    {
        ProgramInterface& program_interface =
            level_interface.GetProgramFromId(program_id);
        if (!program_interface.SerializeEnable())
        {
            continue;
        }
        proto::Program proto_program =
            SerializeProgram(program_interface, level_interface);
        *proto_level.add_programs() = proto_program;
    }
    constexpr std::array<proto::NodeMesh::RenderTimeEnum, 4> kRenderTimes = {
        proto::NodeMesh::PRE_RENDER_TIME,
        proto::NodeMesh::SCENE_RENDER_TIME,
        proto::NodeMesh::POST_PROCESS_TIME,
        proto::NodeMesh::SKYBOX_RENDER_TIME};
    for (const auto render_time : kRenderTimes)
    {
        const auto program_id = level_interface.GetRenderPassProgramId(render_time);
        if (!program_id)
        {
            continue;
        }
        auto* pass = proto_level.add_render_pass_programs();
        pass->set_render_time_enum(render_time);
        pass->set_program_name(level_interface.GetNameFromId(program_id));
        const auto preprocess_id =
            level_interface.GetRenderPassPreprocessProgramId(render_time);
        if (preprocess_id)
        {
            pass->set_preprocess_program_name(
                level_interface.GetNameFromId(preprocess_id));
        }
    }
    *proto_level.mutable_scene_tree() = SerializeSceneTree(level_interface);
    return proto_level;
}

} // End namespace frame::json.
