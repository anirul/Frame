#include "frame/json/serialize_material.h"

namespace frame::json
{

proto::Material SerializeMaterial(
    const MaterialInterface& material_interface,
    const LevelInterface& level_interface)
{
    proto::Material proto_material;
    proto_material.set_name(material_interface.GetName());
    auto maybe_name =
        level_interface.GetNameFromId(material_interface.GetProgramId());
    if (!maybe_name)
    {
        throw std::runtime_error(
            std::format(
                "Program id [{}] doesn't have a valid name?",
                material_interface.GetProgramId()));
    }
    proto_material.set_program_name(maybe_name.value());
    for (const auto& texture_id : material_interface.GetIds())
    {
        auto maybe_name = level_interface.GetNameFromId(texture_id);
        if (!maybe_name)
        {
            throw std::runtime_error(
                std::format("no texture name for if [{}]", texture_id));
        }
        std::string inner_name = material_interface.GetInnerName(texture_id);
        *proto_material.add_texture_names() = maybe_name.value();
        *proto_material.add_inner_names() = inner_name;
    }
    return proto_material;
}

} // End namespace frame::json.
