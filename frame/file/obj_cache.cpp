#include "frame/file/obj_cache.h"

#include <filesystem>
#include <format>
#include <fstream>

#include "frame/logger.h"
#include "frame/proto/obj_cache.pb.h"

namespace frame::file
{
namespace
{
constexpr std::uint32_t kCacheVersion = 1;

Logger& GetLogger()
{
    return Logger::GetInstance();
}

} // namespace

std::optional<ObjCachePayload> LoadObjCache(const ObjCacheMetadata& metadata)
{
    if (metadata.cache_path.empty())
    {
        return std::nullopt;
    }
    if (!std::filesystem::exists(metadata.cache_path))
    {
        return std::nullopt;
    }
    std::ifstream input(metadata.cache_path, std::ios::binary);
    if (!input)
    {
        GetLogger()->warn(
            "Failed to open OBJ cache file {} for reading.",
            metadata.cache_path.string());
        return std::nullopt;
    }
    proto::ObjCache cache_proto;
    if (!cache_proto.ParseFromIstream(&input))
    {
        GetLogger()->warn(
            "Could not parse OBJ cache {}.", metadata.cache_path.string());
        return std::nullopt;
    }
    if (cache_proto.cache_version() != kCacheVersion)
    {
        GetLogger()->info(
            "Ignoring OBJ cache {} due to version mismatch ({} != {}).",
            metadata.cache_path.string(),
            cache_proto.cache_version(),
            kCacheVersion);
        return std::nullopt;
    }
    if (cache_proto.source_size() != metadata.source_size ||
        cache_proto.source_mtime_ns() != metadata.source_mtime_ns ||
        cache_proto.source_relative() != metadata.source_relative ||
        cache_proto.cache_relative() != metadata.cache_relative)
    {
        GetLogger()->info(
            "Ignoring OBJ cache {} due to stale source metadata.",
            metadata.cache_path.string());
        return std::nullopt;
    }

    ObjCachePayload payload;
    payload.hasTextureCoordinates = cache_proto.has_texture_coordinates();
    payload.meshes.reserve(cache_proto.meshes_size());
    for (const auto& mesh_proto : cache_proto.meshes())
    {
        std::vector<ObjVertex> vertices;
        vertices.reserve(mesh_proto.vertices_size());
        for (const auto& vertex_proto : mesh_proto.vertices())
        {
            ObjVertex vertex;
            vertex.point = glm::vec3(
                vertex_proto.position_x(),
                vertex_proto.position_y(),
                vertex_proto.position_z());
            vertex.normal = glm::vec3(
                vertex_proto.normal_x(),
                vertex_proto.normal_y(),
                vertex_proto.normal_z());
            vertex.tex_coord = glm::vec2(
                vertex_proto.tex_coord_u(), vertex_proto.tex_coord_v());
            vertices.push_back(vertex);
        }
        std::vector<int> indices(
            mesh_proto.indices().begin(), mesh_proto.indices().end());
        ObjMesh mesh(
            std::move(vertices),
            std::move(indices),
            mesh_proto.material_id(),
            mesh_proto.has_texture_coordinates());
        payload.meshes.push_back(std::move(mesh));
    }

    payload.materials.reserve(cache_proto.materials_size());
    for (const auto& material_proto : cache_proto.materials())
    {
        ObjMaterial material;
        material.name = material_proto.name();
        material.ambient_vec4 = glm::vec4(
            material_proto.ambient_x(),
            material_proto.ambient_y(),
            material_proto.ambient_z(),
            material_proto.ambient_w());
        material.ambient_str = material_proto.ambient_texture();
        material.diffuse_vec4 = glm::vec4(
            material_proto.diffuse_x(),
            material_proto.diffuse_y(),
            material_proto.diffuse_z(),
            material_proto.diffuse_w());
        material.diffuse_str = material_proto.diffuse_texture();
        material.displacement_str = material_proto.displacement_texture();
        material.roughness_val = material_proto.roughness_value();
        material.roughness_str = material_proto.roughness_texture();
        material.metallic_val = material_proto.metallic_value();
        material.metallic_str = material_proto.metallic_texture();
        material.sheen_val = material_proto.sheen_value();
        material.sheen_str = material_proto.sheen_texture();
        material.emmissive_str = material_proto.emissive_texture();
        material.normal_str = material_proto.normal_texture();
        payload.materials.emplace_back(std::move(material));
    }

    return payload;
}

void SaveObjCache(
    const ObjCacheMetadata& metadata, const ObjCachePayload& payload)
{
    if (metadata.cache_path.empty())
    {
        return;
    }
    std::error_code ec;
    const auto parent = metadata.cache_path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            GetLogger()->warn(
                "Failed to create OBJ cache directory {}: {}",
                parent.string(),
                ec.message());
            return;
        }
    }

    proto::ObjCache cache_proto;
    cache_proto.set_cache_version(kCacheVersion);
    cache_proto.set_cache_relative(metadata.cache_relative);
    cache_proto.set_source_relative(metadata.source_relative);
    cache_proto.set_source_size(metadata.source_size);
    cache_proto.set_source_mtime_ns(metadata.source_mtime_ns);
    cache_proto.set_has_texture_coordinates(payload.hasTextureCoordinates);

    for (const auto& mesh : payload.meshes)
    {
        auto* mesh_proto = cache_proto.add_meshes();
        mesh_proto->set_material_id(mesh.GetMaterialId());
        mesh_proto->set_has_texture_coordinates(mesh.HasTextureCoordinates());
        for (const auto& index : mesh.GetIndices())
        {
            mesh_proto->add_indices(index);
        }
        for (const auto& vertex : mesh.GetVertices())
        {
            auto* vertex_proto = mesh_proto->add_vertices();
            vertex_proto->set_position_x(vertex.point.x);
            vertex_proto->set_position_y(vertex.point.y);
            vertex_proto->set_position_z(vertex.point.z);
            vertex_proto->set_normal_x(vertex.normal.x);
            vertex_proto->set_normal_y(vertex.normal.y);
            vertex_proto->set_normal_z(vertex.normal.z);
            vertex_proto->set_tex_coord_u(vertex.tex_coord.x);
            vertex_proto->set_tex_coord_v(vertex.tex_coord.y);
        }
    }

    for (const auto& material : payload.materials)
    {
        auto* material_proto = cache_proto.add_materials();
        material_proto->set_name(material.name);
        material_proto->set_ambient_x(material.ambient_vec4.x);
        material_proto->set_ambient_y(material.ambient_vec4.y);
        material_proto->set_ambient_z(material.ambient_vec4.z);
        material_proto->set_ambient_w(material.ambient_vec4.w);
        material_proto->set_ambient_texture(material.ambient_str);
        material_proto->set_diffuse_x(material.diffuse_vec4.x);
        material_proto->set_diffuse_y(material.diffuse_vec4.y);
        material_proto->set_diffuse_z(material.diffuse_vec4.z);
        material_proto->set_diffuse_w(material.diffuse_vec4.w);
        material_proto->set_diffuse_texture(material.diffuse_str);
        material_proto->set_displacement_texture(material.displacement_str);
        material_proto->set_roughness_value(material.roughness_val);
        material_proto->set_roughness_texture(material.roughness_str);
        material_proto->set_metallic_value(material.metallic_val);
        material_proto->set_metallic_texture(material.metallic_str);
        material_proto->set_sheen_value(material.sheen_val);
        material_proto->set_sheen_texture(material.sheen_str);
        material_proto->set_emissive_texture(material.emmissive_str);
        material_proto->set_normal_texture(material.normal_str);
    }

    std::ofstream output(
        metadata.cache_path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        GetLogger()->warn(
            "Failed to open OBJ cache file {} for writing.",
            metadata.cache_path.string());
        return;
    }
    if (!cache_proto.SerializeToOstream(&output))
    {
        GetLogger()->warn(
            "Failed to serialize OBJ cache {}.", metadata.cache_path.string());
    }
}

} // namespace frame::file
