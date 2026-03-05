#include "frame/vulkan/json/parse_material.h"

#include <format>

#include "frame/vulkan/material.h"

namespace frame::vulkan::json
{

std::unique_ptr<frame::MaterialInterface> ParseMaterial(
    const frame::proto::Material& proto_material,
    frame::LevelInterface& /*level*/)
{
    auto material = std::make_unique<frame::vulkan::Material>();
    material->SetName(proto_material.name());
    material->SetSerializeEnable(true);

    auto material_proto_copy = proto_material;
    material->FromProto(std::move(material_proto_copy));

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
