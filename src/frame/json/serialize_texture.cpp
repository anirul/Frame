#include "frame/json/serialize_texture.h"

#include "frame/file/file_system.h"
#include "frame/json/serialize_uniform.h"
#include "frame/opengl/cubemap.h"

namespace frame::json
{

namespace
{

proto::Texture SerializeTextureFromParameter(
    const TextureParameter& texture_parameter)
{
    proto::Texture proto_texture;
    *proto_texture.mutable_pixel_element_size() =
        texture_parameter.pixel_element_size;
    *proto_texture.mutable_pixel_structure() =
        texture_parameter.pixel_structure;
    if (!texture_parameter.file_name.empty())
    {
        proto_texture.set_file_name(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.file_name)));
        return proto_texture;
    }
    if (!texture_parameter.array_file_names[0].empty())
    {
        if (texture_parameter.array_file_names.size() != 6)
        {
            throw std::runtime_error(
                std::format(
                    "Array files should have 6 files not {}?",
                    texture_parameter.array_file_names.size()));
        }
        proto::CubeMapFiles proto_cubemap_files;
        proto_cubemap_files.set_positive_x(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[0])));
        proto_cubemap_files.set_negative_x(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[1])));
        proto_cubemap_files.set_positive_y(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[2])));
        proto_cubemap_files.set_negative_y(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[3])));
        proto_cubemap_files.set_positive_z(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[4])));
        proto_cubemap_files.set_negative_z(
            file::PurifyFilePath(
                std::filesystem::path(texture_parameter.array_file_names[5])));
        *proto_texture.mutable_file_names() = proto_cubemap_files;
        return proto_texture;
    }
    if (texture_parameter.relative_size != glm::uvec2(0, 0))
    {
        *proto_texture.mutable_size() =
            SerializeSize(texture_parameter.relative_size);
    }
    else
    {
        *proto_texture.mutable_size() = SerializeSize(texture_parameter.size);
    }
    return proto_texture;
}

} // End anonymous namespace.

proto::Texture SerializeTexture(TextureInterface& texture_interface)
{
    TextureParameter texture_parameter =
        texture_interface.GetTextureParameter();
    proto::Texture proto_texture =
        SerializeTextureFromParameter(texture_interface.GetTextureParameter());
    proto_texture.set_name(texture_interface.GetName());
    proto_texture.set_cubemap(texture_interface.IsCubeMap());
    // TODO(anirul): Get the file names?
    return proto_texture;
}

} // End namespace frame::json.
