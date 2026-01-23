#include "frame/file/obj.h"

#include <chrono>
#include <fstream>
#include <numeric>
#include <string_view>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <format>
#include <tiny_obj_loader.h>

#include "frame/file/file_system.h"
#include "frame/file/obj_cache.h"

namespace frame::file
{

namespace
{

std::filesystem::path StripAssetPrefix(std::filesystem::path input)
{
    if (input.is_absolute())
    {
        return input;
    }
    const std::string generic = input.generic_string();
    constexpr std::string_view kPrefix = "asset/";
    if (generic.rfind(kPrefix, 0) == 0)
    {
        return std::filesystem::path(generic.substr(kPrefix.size()));
    }
    return input;
}

} // namespace

Obj::Obj(std::filesystem::path file_name)
{
    bool source_available = true;
    std::filesystem::path absolute_path;
    try
    {
        const auto resolved_path = frame::file::FindFile(file_name);
        absolute_path =
            std::filesystem::absolute(resolved_path).lexically_normal();
    }
    catch (const std::exception&)
    {
        source_available = false;
        const auto asset_root = frame::file::FindDirectory("asset");
        auto relative = StripAssetPrefix(file_name);
        absolute_path = (asset_root / relative).lexically_normal();
    }

    std::optional<ObjCacheMetadata> cache_metadata;
    std::optional<std::filesystem::file_time_type> source_write_time;
    std::filesystem::path cache_path;
    std::string cache_relative;
    bool cache_path_ready = false;
    try
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        const auto cache_root =
            (asset_root / "cache").lexically_normal();
        std::error_code relative_error;
        auto relative = std::filesystem::relative(
            absolute_path, asset_root, relative_error);
        if (relative_error)
        {
            relative = absolute_path.filename();
        }
        cache_path = (cache_root / relative).lexically_normal();
        cache_path.replace_extension(".objpb");
        cache_relative = frame::file::PurifyFilePath(cache_path);
        cache_path_ready = true;
    }
    catch (const std::exception& exception)
    {
        logger_->warn("OBJ cache disabled: {}", exception.what());
    }

    if (source_available)
    {
        std::error_code metadata_error;
        const auto source_size =
            std::filesystem::file_size(absolute_path, metadata_error);
        if (!metadata_error && cache_path_ready)
        {
            auto write_time =
                std::filesystem::last_write_time(absolute_path, metadata_error);
            if (!metadata_error)
            {
                source_write_time = write_time;
                const auto mtime_ns =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        write_time.time_since_epoch())
                        .count();
                ObjCacheMetadata metadata;
                metadata.source_relative =
                    frame::file::PurifyFilePath(absolute_path);
                metadata.source_size = static_cast<std::uint64_t>(source_size);
                metadata.source_mtime_ns = static_cast<std::uint64_t>(mtime_ns);
                metadata.cache_path = cache_path;
                metadata.cache_relative = cache_relative;
                cache_metadata = std::move(metadata);
            }
        }
        else
        {
            logger_->info(
                "OBJ cache disabled for {}: unable to inspect source file.",
                absolute_path.string());
        }
    }

    if (cache_metadata)
    {
        if (auto cached = LoadObjCache(*cache_metadata))
        {
            has_texture_coordinates_ = cached->hasTextureCoordinates;
            meshes_ = std::move(cached->meshes);
            materials_ = std::move(cached->materials);
            logger_->info(
                "Loaded OBJ cache {}.", cache_metadata->cache_relative);
            return;
        }
    }
    if (source_available && cache_path_ready && source_write_time)
    {
        std::error_code cache_time_error;
        const auto cache_write_time =
            std::filesystem::last_write_time(cache_path, cache_time_error);
        if (!cache_time_error && cache_write_time >= *source_write_time)
        {
            if (auto cached = LoadObjCacheRelaxed(cache_path))
            {
                has_texture_coordinates_ = cached->hasTextureCoordinates;
                meshes_ = std::move(cached->meshes);
                materials_ = std::move(cached->materials);
                logger_->info(
                    "Loaded OBJ cache {} (source present).",
                    cache_relative);
                return;
            }
        }
    }
    else if (!source_available && cache_path_ready)
    {
        if (auto cached = LoadObjCacheRelaxed(cache_path))
        {
            has_texture_coordinates_ = cached->hasTextureCoordinates;
            meshes_ = std::move(cached->meshes);
            materials_ = std::move(cached->materials);
            logger_->info(
                "Loaded OBJ cache {} (source missing).",
                cache_relative);
            return;
        }
    }

    if (!source_available)
    {
        throw std::runtime_error(std::format(
            "OBJ source [{}] not found and cache missing.",
            absolute_path.string()));
    }

#ifdef TINY_OBJ_LOADER_V2
    tinyobj::ObjReaderConfig reader_config;
    const auto pair = SplitFileDirectory(absolute_path);
    reader_config.mtl_search_path = pair.first;
    tinyobj::ObjReader reader;
    std::string total_path = absolute_path.string();
    if (!reader.ParseFromFile(total_path))
    {
        if (!reader.Error().empty())
        {
            throw std::runtime_error(reader.Error());
        }
        throw std::runtime_error(
            std::format("Unknown error parsing file [{}].", total_path));
    }

    logger_->info("Opening OBJ File [{}].", total_path);
    if (!reader.Warning().empty())
    {
        logger_->warn(
            "Warning parsing file {}: {}", total_path, reader.Warning());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
#else
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    const auto directory = absolute_path.parent_path();
    const auto file = absolute_path.filename();

    std::string err;
    std::string warn;
    bool ret = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        absolute_path.string().c_str(),
        directory.string().c_str());
    if (!warn.empty())
    {
        logger_->warn("Warning parsing file {}: {}", file_name.string(), warn);
    }
    if (!err.empty())
    {
        logger_->error("Error parsing file {}: {}", file_name.string(), err);
        throw std::runtime_error(err);
    }
#endif // TINY_OBJ_LOADER_V2
    has_texture_coordinates_ = !attrib.texcoords.empty();

    if (shapes.size() == 0)
    {
        std::vector<ObjVertex> points;
        int material_id = 0;
        for (std::vector<float>::iterator ite = attrib.vertices.begin();
             ite != attrib.vertices.end();
             ite += 3)
        {
            ObjVertex obj_vertex;
            obj_vertex.point.x = *ite;
            obj_vertex.point.y = *(ite + 1);
            obj_vertex.point.z = *(ite + 2);
            points.push_back(obj_vertex);
        }
        std::vector<int> indices;
        indices.resize(attrib.vertices.size());
        std::iota(indices.begin(), indices.end(), 1);
        ObjMesh mesh(points, indices, material_id, has_texture_coordinates_);
        meshes_.push_back(mesh);
    }
    else
    {
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            std::vector<ObjVertex> points;
            std::vector<int> indices;
            int material_id = 0;

            // Loop over faces(polygon) this should be triangles?
            size_t index_offset = 0;
            for (std::size_t f = 0; f < shapes[s].mesh.num_face_vertices.size();
                 f++)
            {
                // This SHOULD be 3!
                int fv = shapes[s].mesh.num_face_vertices[f];
                if (fv != 3)
                {
                    throw std::runtime_error(
                        std::format(
                            "The face should be 3 in size now {}.", fv));
                }
                // Loop over vertices in the face.
                for (std::size_t v = 0; v < fv; v++)
                {
                    ObjVertex vertex{};
                    // access to vertex
                    tinyobj::index_t idx =
                        shapes[s].mesh.indices[index_offset + v];
                    vertex.point.x = attrib.vertices[3 * idx.vertex_index + 0];
                    vertex.point.y = attrib.vertices[3 * idx.vertex_index + 1];
                    vertex.point.z = attrib.vertices[3 * idx.vertex_index + 2];
                    if (idx.normal_index >= 0)
                    {
                        vertex.normal.x =
                            attrib.normals[3 * idx.normal_index + 0];
                        vertex.normal.y =
                            attrib.normals[3 * idx.normal_index + 1];
                        vertex.normal.z =
                            attrib.normals[3 * idx.normal_index + 2];
                    }
                    if (idx.texcoord_index >= 0)
                    {
                        vertex.tex_coord.x =
                            attrib.texcoords[2 * idx.texcoord_index + 0];
                        vertex.tex_coord.y =
                            attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    points.push_back(vertex);
                    indices.push_back(static_cast<int>(indices.size()));
                }
                index_offset += fv;

                // per-face material
                if (material_id)
                {
                    assert(material_id == shapes[s].mesh.material_ids[f]);
                }
                material_id = shapes[s].mesh.material_ids[f];
            }
            ObjMesh mesh(
                points, indices, material_id, has_texture_coordinates_);
            meshes_.push_back(mesh);
        }
    }

    for (const auto& material : materials)
    {
        ObjMaterial obj_material{};
        obj_material.name = material.name;
        obj_material.ambient_str = material.ambient_texname;
        obj_material.ambient_vec4 = glm::vec4(
            material.ambient[0],
            material.ambient[1],
            material.ambient[2],
            material.dissolve);
        obj_material.diffuse_str = material.diffuse_texname;
        obj_material.diffuse_vec4 = glm::vec4(
            material.diffuse[0],
            material.diffuse[1],
            material.diffuse[2],
            material.dissolve);
        obj_material.displacement_str = material.displacement_texname;
        obj_material.emmissive_str = material.emissive_texname;
        obj_material.metallic_str = material.metallic_texname;
        obj_material.metallic_val = material.metallic;
        obj_material.normal_str = material.normal_texname;
        obj_material.roughness_str = material.roughness_texname;
        obj_material.roughness_val = material.roughness;
        obj_material.sheen_str = material.sheen_texname;
        obj_material.sheen_val = material.sheen;
        materials_.emplace_back(obj_material);
    }

    if (cache_metadata)
    {
        ObjCachePayload payload;
        payload.hasTextureCoordinates = has_texture_coordinates_;
        payload.meshes = meshes_;
        payload.materials = materials_;
        SaveObjCache(*cache_metadata, payload);
        logger_->info("Saved OBJ cache {}.", cache_metadata->cache_relative);
    }
}

Obj::~Obj() = default;

} // End namespace frame::file.
