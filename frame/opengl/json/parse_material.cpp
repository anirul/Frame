#include "frame/opengl/json/parse_material.h"

#include "frame/opengl/material.h"

namespace frame::json
{

std::unique_ptr<frame::MaterialInterface> ParseMaterialOpenGL(
    const frame::proto::Material& proto_material, LevelInterface& level)
{
    if (proto_material.program_name() == "DisplayProgram")
    {
        return nullptr;
    }
    const std::size_t texture_size = proto_material.texture_names_size();
    const std::size_t inner_size = proto_material.inner_names_size();
    if (texture_size != inner_size)
    {
        throw std::runtime_error(
            std::format(
                "Not the same size for texture and inner names: {} != {}.",
                texture_size,
                inner_size));
    }
    auto material = std::make_unique<frame::opengl::Material>();
    material->SetName(proto_material.name());
    material->SetSerializeEnable(true);
    if (proto_material.program_name().empty())
    {
        throw std::runtime_error(
            std::format("No program name in {}.", proto_material.name()));
    }
    material->GetData().set_program_name(proto_material.program_name());
    auto maybe_program_id = level.GetIdFromName(proto_material.program_name());
    if (maybe_program_id)
    {
        EntityId program_id = maybe_program_id;
        material->SetProgramId(program_id);
    }
    if (!proto_material.preprocess_program_name().empty())
    {
        material->GetData().set_preprocess_program_name(
            proto_material.preprocess_program_name());
        material->SetPreprocessProgramName(
            proto_material.preprocess_program_name());
        auto maybe_preprocess_id =
            level.GetIdFromName(proto_material.preprocess_program_name());
        if (maybe_preprocess_id)
        {
            material->SetPreprocessProgramId(maybe_preprocess_id);
        }
    }
    for (int i = 0; i < inner_size; ++i)
    {
        auto maybe_texture_id =
            level.GetIdFromName(proto_material.texture_names(i));
        if (!maybe_texture_id)
            return nullptr;
        EntityId texture_id = maybe_texture_id;
        material->AddTextureId(texture_id, proto_material.inner_names(i));
    }
    const std::size_t buffer_size = proto_material.buffer_names_size();
    const std::size_t inner_buffer_size =
        proto_material.inner_buffer_names_size();
    if (buffer_size != inner_buffer_size)
    {
        throw std::runtime_error(std::format(
            "Not the same size for buffer and inner names: {} != {}.",
            buffer_size,
            inner_buffer_size));
    }
    for (int i = 0; i < buffer_size; ++i)
    {
        material->AddBufferName(
            proto_material.buffer_names(i),
            proto_material.inner_buffer_names(i));
    }
    const std::size_t node_size = proto_material.node_names_size();
    const std::size_t inner_node_size =
        proto_material.inner_node_names_size();
    if (node_size != inner_node_size)
    {
        throw std::runtime_error(std::format(
            "Not the same size for node and inner names: {} != {}.",
            node_size,
            inner_node_size));
    }
    for (int i = 0; i < node_size; ++i)
    {
        material->AddNodeName(
            proto_material.node_names(i),
            proto_material.inner_node_names(i));
    }
    return material;
}

} // End namespace frame::json.
