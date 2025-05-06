#include "frame/json/serialize_program.h"
#include "frame/json/proto.h"
#include "frame/json/serialize_uniform.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"
#include <format>

namespace frame::proto
{

proto::Program SerializeProgram(
    const frame::ProgramInterface& program_interface,
    const frame::LevelInterface& level_interface)
{
    proto::Program proto_program;
    proto_program.set_name(program_interface.GetName());
    for (const auto& input_texture_id : program_interface.GetInputTextureIds())
    {
        auto maybe_name = level_interface.GetNameFromId(input_texture_id);
        if (!maybe_name)
        {
            throw std::runtime_error(
                std::format(
                    "Invalid texture name at id[{}]?", input_texture_id));
        }
        *proto_program.add_input_texture_names() = maybe_name.value();
    }
    for (const auto& output_texture_id :
         program_interface.GetOutputTextureIds())
    {
        auto maybe_name = level_interface.GetNameFromId(output_texture_id);
        if (!maybe_name)
        {
            throw std::runtime_error(
                std::format(
                    "Invalid texture name at id[{}]?", output_texture_id));
        }
        *proto_program.add_output_texture_names() = maybe_name.value();
    }
    proto::SceneType proto_scene_type;
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
        auto maybe_name = level_interface.GetNameFromId(
            level_interface.GetDefaultRootSceneNodeId());
        if (!maybe_name)
        {
            throw std::runtime_error(
                std::format(
                    "No root element name id[{}]?",
                    level_interface.GetDefaultRootSceneNodeId()));
        }
        proto_program.set_input_scene_root_name(maybe_name.value());
    }
    else
    {
        proto_scene_type.set_value(proto::SceneType::NONE);
    }
    *proto_program.mutable_input_scene_type() = proto_scene_type;
    for (const auto& uniform_name : program_interface.GetUniformNameList())
    {
        proto::Uniform proto_uniform;
        proto_uniform =
            SerializeUniform(program_interface.GetUniform(uniform_name));
        *proto_program.add_parameters() = proto_uniform;
    }
    return proto_program;
}

} // namespace frame::proto.
