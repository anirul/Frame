#include "frame/json/level_data.h"

namespace frame::json
{

LevelData BuildLevelData(
    glm::uvec2 /*size*/,
    const proto::Level& proto_level,
    const std::filesystem::path& asset_root)
{
    LevelData data;
    data.proto = proto_level;
    data.asset_root = asset_root;

    for (const auto& proto_texture : proto_level.textures())
    {
        TextureInfo texture_info;
        texture_info.name = proto_texture.name();
        texture_info.element_size = proto_texture.pixel_element_size();
        texture_info.structure = proto_texture.pixel_structure();
        if (proto_texture.has_size())
        {
            texture_info.size = glm::uvec2(
                static_cast<std::uint32_t>(proto_texture.size().x()),
                static_cast<std::uint32_t>(proto_texture.size().y()));
        }
        data.textures.push_back(std::move(texture_info));
    }

    for (const auto& proto_program : proto_level.programs())
    {
        ProgramInfo program_info;
        program_info.name = proto_program.name();
        program_info.vertex_shader = proto_program.shader_vertex();
        program_info.fragment_shader = proto_program.shader_fragment();
        data.programs.push_back(std::move(program_info));
    }

    for (const auto& proto_material : proto_level.materials())
    {
        MaterialInfo material_info;
        material_info.name = proto_material.name();
        material_info.program_name = proto_material.program_name();
        material_info.texture_names.assign(
            proto_material.texture_names().begin(),
            proto_material.texture_names().end());
        data.materials.push_back(std::move(material_info));
    }

    for (const auto& node : proto_level.scene_tree().node_static_meshes())
    {
        if (node.mesh_enum() == frame::proto::NodeStaticMesh::QUAD)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                -0.5f, 0.5f, 0.0f};
            mesh_info.uvs = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f};
            mesh_info.indices = {0, 1, 2, 2, 3, 0};
            data.meshes.push_back(std::move(mesh_info));
        }
        else if (node.mesh_enum() == frame::proto::NodeStaticMesh::CUBE)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                // Front.
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                -0.5f, 0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                // Back.
                -0.5f, -0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f,
                // Left.
                -0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                // Right.
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                // Bottom.
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f,
                -0.5f, -0.5f, -0.5f,
                // Top.
                -0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, -0.5f};
            mesh_info.uvs = {
                // Front.
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                // Back.
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                // Left.
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
                // Right.
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
                // Bottom.
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 0.0f,
                0.0f, 1.0f,
                // Top.
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 0.0f,
                0.0f, 1.0f};
            mesh_info.indices.resize(36);
            std::iota(mesh_info.indices.begin(), mesh_info.indices.end(), 0);
            data.meshes.push_back(std::move(mesh_info));
        }
    }

    return data;
}

} // namespace frame::json
