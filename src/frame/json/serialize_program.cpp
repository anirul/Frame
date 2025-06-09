#include "frame/json/serialize_program.h"
#include "frame/json/proto.h"
#include "frame/json/serialize_uniform.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"
#include <format>

namespace frame::json
{

proto::Program SerializeProgram(
    const frame::ProgramInterface& program_interface,
    const frame::LevelInterface& level_interface)
{
    proto::Program proto_program;
    proto_program.set_name(program_interface.GetName());
    proto_program.set_shader(program_interface.GetData().shader());
    for (const auto& input_texture_id : program_interface.GetInputTextureIds())
    {
        *proto_program.add_input_texture_names() =
            level_interface.GetNameFromId(input_texture_id);
    }
    for (const auto& output_texture_id :
         program_interface.GetOutputTextureIds())
    {
        *proto_program.add_output_texture_names() =
            level_interface.GetNameFromId(output_texture_id);
    }
    proto::SceneType proto_scene_type;
    if (program_interface.GetTemporarySceneRoot() == "")
    {
        auto id = program_interface.GetSceneRoot();
        if (id == level_interface.GetDefaultStaticMeshQuadId())
        {
            proto_scene_type.set_value(proto::SceneType::QUAD);
        }
        else if (id == level_interface.GetDefaultStaticMeshCubeId())
        {
            proto_scene_type.set_value(proto::SceneType::CUBE);
        }
        else if (id == level_interface.GetDefaultRootSceneNodeId())
        {
            proto_scene_type.set_value(proto::SceneType::SCENE);
            proto_program.set_input_scene_root_name(
                level_interface.GetNameFromId(
                    level_interface.GetDefaultRootSceneNodeId()));
        }
        else
        {
            proto_scene_type.set_value(proto::SceneType::NONE);
        }
    }
    else
    {
        proto_scene_type.set_value(proto::SceneType::SCENE);
        *proto_program.mutable_input_scene_root_name() =
            program_interface.GetTemporarySceneRoot();
    }
    *proto_program.mutable_input_scene_type() = proto_scene_type;
    for (const auto& uniform_name : program_interface.GetUniformNameList())
    {
        proto::Uniform proto_uniform;
        if (program_interface.HasUniformEnum(uniform_name))
        {
            proto_uniform.set_name(uniform_name);
            proto_uniform.set_uniform_enum(
                program_interface.GetUniformEnum(uniform_name));
        }
        else
        {
            proto_uniform = SerializeUniform(
                program_interface.GetUniform(uniform_name), level_interface);
        }
        // Could return an empty name in case it was found in the materials.
        if (!proto_uniform.name().empty())
        {
            *proto_program.add_uniforms() = proto_uniform;
        }
    }
    return proto_program;
}

} // namespace frame::json.
