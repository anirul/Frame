#include "frame/opengl/json/parse_material.h"

#include <format>

#include "frame/opengl/material.h"

namespace frame::json
{

std::unique_ptr<frame::MaterialInterface> ParseMaterialOpenGL(
    const frame::proto::Material& proto_material, LevelInterface& /*level*/)
{
    auto material = std::make_unique<frame::opengl::Material>();
    material->SetName(proto_material.name());
    material->SetSerializeEnable(true);

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
