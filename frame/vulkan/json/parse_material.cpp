#include "frame/vulkan/json/parse_material.h"

#include <format>
#include <stdexcept>

#include "frame/vulkan/material.h"

namespace frame::vulkan::json
{

std::unique_ptr<frame::MaterialInterface> ParseMaterial(
    const frame::proto::Material& proto_material,
    frame::LevelInterface& level)
{
    if (proto_material.program_name().empty())
    {
        throw std::runtime_error(
            std::format("No program name in {}.", proto_material.name()));
    }

    auto material = std::make_unique<frame::vulkan::Material>();
    material->SetName(proto_material.name());
    material->SetSerializeEnable(true);

    auto material_proto_copy = proto_material;
    material->FromProto(std::move(material_proto_copy));

    if (auto program_id = level.GetIdFromName(proto_material.program_name());
        program_id != NullId)
    {
        material->SetProgramId(program_id);
    }
    if (!proto_material.preprocess_program_name().empty())
    {
        if (auto preprocess_id =
                level.GetIdFromName(proto_material.preprocess_program_name());
            preprocess_id != NullId)
        {
            material->SetPreprocessProgramId(preprocess_id);
        }
    }

    if (proto_material.texture_names_size() !=
        proto_material.inner_names_size())
    {
        throw std::runtime_error(std::format(
            "Texture and inner name count mismatch: {} != {}.",
            proto_material.texture_names_size(),
            proto_material.inner_names_size()));
    }

    for (int i = 0; i < proto_material.texture_names_size(); ++i)
    {
        const auto texture_name = proto_material.texture_names(i);
        const auto inner_name = proto_material.inner_names(i);
        auto texture_id = level.GetIdFromName(texture_name);
        if (texture_id == NullId)
        {
            throw std::runtime_error(
                std::format("Texture {} not found for material {}.",
                    texture_name,
                    proto_material.name()));
        }
        material->AddTextureId(texture_id, inner_name);
    }

    if (proto_material.buffer_names_size() !=
        proto_material.inner_buffer_names_size())
    {
        throw std::runtime_error(std::format(
            "Buffer name mismatch: {} != {}.",
            proto_material.buffer_names_size(),
            proto_material.inner_buffer_names_size()));
    }
    for (int i = 0; i < proto_material.buffer_names_size(); ++i)
    {
        material->AddBufferName(
            proto_material.buffer_names(i),
            proto_material.inner_buffer_names(i));
    }

    if (proto_material.node_names_size() !=
        proto_material.inner_node_names_size())
    {
        throw std::runtime_error(std::format(
            "Node name mismatch: {} != {}.",
            proto_material.node_names_size(),
            proto_material.inner_node_names_size()));
    }
    for (int i = 0; i < proto_material.node_names_size(); ++i)
    {
        material->AddNodeName(
            proto_material.node_names(i),
            proto_material.inner_node_names(i));
    }

    return material;
}

} // namespace frame::vulkan::json
