#include "frame/json/serialize_material.h"

namespace frame::json
{

proto::Material SerializeMaterial(
    const MaterialInterface& material_interface,
    const LevelInterface& level_interface)
{
    proto::Material proto_material;
    proto_material.set_name(material_interface.GetName());
    proto_material.set_program_name(
        level_interface.GetNameFromId(material_interface.GetProgramId()));
    for (const auto texture_id : material_interface.GetTextureIds())
    {
        std::string inner_name = material_interface.GetInnerName(texture_id);
        *proto_material.add_texture_names() =
            level_interface.GetNameFromId(texture_id);
        *proto_material.add_inner_names() = inner_name;
    }
    for (const auto& name : material_interface.GetBufferNames())
    {
        std::string inner_name = material_interface.GetInnerBufferName(name);
        *proto_material.add_buffer_names() = name;
        *proto_material.add_inner_buffer_names() = inner_name;
    }
    return proto_material;
}

} // End namespace frame::json.
