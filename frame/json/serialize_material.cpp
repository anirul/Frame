#include "frame/json/serialize_material.h"

namespace frame::json
{

proto::Material SerializeMaterial(
    const MaterialInterface& material_interface,
    const LevelInterface& level_interface)
{
    proto::Material proto_material;
    proto_material.set_name(material_interface.GetName());
    (void)level_interface;
    for (const auto& name : material_interface.GetBufferNames())
    {
        std::string inner_name = material_interface.GetInnerBufferName(name);
        *proto_material.add_buffer_names() = name;
        *proto_material.add_inner_buffer_names() = inner_name;
    }
    for (const auto& name : material_interface.GetNodeNames())
    {
        std::string inner_name = material_interface.GetInnerNodeName(name);
        *proto_material.add_node_names() = name;
        *proto_material.add_inner_node_names() = inner_name;
    }
    return proto_material;
}

} // End namespace frame::json.
