#include "frame/opengl/file/load_static_mesh.h"

#include <format>
#include <chrono>
#include <string_view>
#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/file/obj.h"
#include "frame/file/ply.h"
#include "frame/logger.h"
#include "frame/opengl/buffer.h"
#include "frame/bvh.h"
#include "frame/bvh_cache.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/static_mesh.h"

namespace frame::opengl::file
{

namespace
{

std::filesystem::path ResolveAssetPath(std::filesystem::path file)
{
    if (file.is_absolute())
    {
        return file;
    }
    const std::string generic = file.generic_string();
    constexpr std::string_view kPrefix = "asset/";
    if (generic.rfind(kPrefix, 0) == 0)
    {
        file = std::filesystem::path(generic.substr(kPrefix.size()));
    }
    const auto asset_root = frame::file::FindDirectory("asset");
    return (asset_root / file).lexically_normal();
}

template <typename T>
std::optional<EntityId> CreateBufferInLevel(
    LevelInterface& level,
    const std::vector<T>& vec,
    const std::string& desc,
    const BufferTypeEnum buffer_type = BufferTypeEnum::ARRAY_BUFFER,
    const BufferUsageEnum buffer_usage = BufferUsageEnum::STATIC_DRAW)
{
    auto buffer = std::make_unique<Buffer>(buffer_type, buffer_usage);
    if (!buffer)
        throw std::runtime_error("No buffer create!");
    // Buffer initialization.
    buffer->Bind();
    buffer->Copy(vec.size() * sizeof(T), vec.data());
    buffer->UnBind();
    buffer->SetName(desc);
    return level.AddBuffer(std::move(buffer));
}

std::unique_ptr<TextureInterface> LoadTextureFromString(
    const std::string& str,
    const proto::PixelElementSize pixel_element_size,
    const proto::PixelStructure pixel_structure)
{
    return std::make_unique<Texture>(
        ResolveAssetPath(str),
        pixel_element_size,
        pixel_structure);
}

std::optional<EntityId> LoadMaterialFromObj(
    LevelInterface& level, const frame::file::ObjMaterial& material_obj)
{
    // Load textures.
    auto color = (material_obj.ambient_str.empty())
                     ? LoadTextureFromVec4(material_obj.ambient_vec4)
                     : LoadTextureFromString(
                           material_obj.ambient_str,
                           json::PixelElementSize_BYTE(),
                           json::PixelStructure_RGB());
    if (!color)
        return std::nullopt;
    auto normal = (material_obj.normal_str.empty())
                      ? LoadTextureFromVec4(glm::vec4(0.f, 0.f, 0.f, 1.f))
                      : LoadTextureFromString(
                            material_obj.normal_str,
                            json::PixelElementSize_BYTE(),
                            json::PixelStructure_RGB());
    if (!normal)
        return std::nullopt;
    auto roughness = (material_obj.roughness_str.empty())
                         ? LoadTextureFromFloat(material_obj.roughness_val)
                         : LoadTextureFromString(
                               material_obj.roughness_str,
                               json::PixelElementSize_BYTE(),
                               json::PixelStructure_GREY());
    if (!roughness)
        return std::nullopt;
    auto metallic = (material_obj.metallic_str.empty())
                        ? LoadTextureFromFloat(material_obj.metallic_val)
                        : LoadTextureFromString(
                              material_obj.metallic_str,
                              json::PixelElementSize_BYTE(),
                              json::PixelStructure_GREY());
    if (!metallic)
        return std::nullopt;
    // Create names for textures.
    auto color_name = std::format("{}.Color", material_obj.name);
    auto normal_name = std::format("{}.Normal", material_obj.name);
    auto roughness_name = std::format("{}.Roughness", material_obj.name);
    auto metallic_name = std::format("{}.Metallic", material_obj.name);
    // Add texture to the level.
    color->SetName(color_name);
    auto color_id = level.AddTexture(std::move(color));
    if (!color_id)
        return std::nullopt;
    normal->SetName(normal_name);
    auto normal_id = level.AddTexture(std::move(normal));
    if (!normal_id)
        return std::nullopt;
    roughness->SetName(roughness_name);
    auto roughness_id = level.AddTexture(std::move(roughness));
    if (!roughness_id)
        return std::nullopt;
    metallic->SetName(metallic_name);
    auto metallic_id = level.AddTexture(std::move(metallic));
    if (!metallic_id)
        return std::nullopt;
    // Create the material.
    std::unique_ptr<MaterialInterface> material =
        std::make_unique<opengl::Material>();
    // Add texture to the material.
    material->AddTextureId(color_id, color_name);
    material->AddTextureId(normal_id, normal_name);
    material->AddTextureId(roughness_id, roughness_name);
    material->AddTextureId(metallic_id, metallic_name);
    // Finally add the material to the level.
    material->SetName(material_obj.name);
    return level.AddMaterial(std::move(material));
}

std::pair<EntityId, EntityId> LoadStaticMeshFromObj(
    LevelInterface& level,
    const frame::file::ObjMesh& mesh_obj,
    const std::string& name,
    const std::vector<EntityId> material_ids,
    int counter,
    const std::optional<frame::BvhCacheMetadata>& cache_metadata,
    proto::NodeStaticMesh::AccelerationStructureEnum acceleration_structure_enum)
{
    std::vector<float> points;
    std::vector<float> normals;
    std::vector<float> textures;
    const auto& vertices = mesh_obj.GetVertices();
    // TODO(anirul): could probably short this out!
    for (const auto& vertice : vertices)
    {
        points.push_back(vertice.point.x);
        points.push_back(vertice.point.y);
        points.push_back(vertice.point.z);
        normals.push_back(vertice.normal.x);
        normals.push_back(vertice.normal.y);
        normals.push_back(vertice.normal.z);
    }
    const auto& indices = mesh_obj.GetIndices();
    // Check if texture coordinates are present.
    if (mesh_obj.HasTextureCoordinates())
    {
        for (const auto& vertice : vertices)
        {
            textures.push_back(vertice.tex_coord.x);
            textures.push_back(vertice.tex_coord.y);
        }
    }
    else
    {
        Logger::GetInstance()->warn(
            "No texture coordinates found for mesh {}, using normals.", name);
        for (const auto& vertice : vertices)
        {
            textures.push_back(vertice.normal.x);
            textures.push_back(vertice.normal.y);
        }
    }

    // Point buffer initialization.
    auto maybe_point_buffer_id = CreateBufferInLevel(
        level, points, std::format("{}.{}.point", name, counter));
    if (!maybe_point_buffer_id)
        return {NullId, NullId};
    EntityId point_buffer_id = maybe_point_buffer_id.value();

    // Normal buffer initialization.
    EntityId normal_buffer_id = NullId;
    if (!normals.empty())
    {
        auto maybe_normal_buffer_id = CreateBufferInLevel(
            level, normals, std::format("{}.{}.normal", name, counter));
        if (maybe_normal_buffer_id)
            normal_buffer_id = maybe_normal_buffer_id.value();
    }

    // Texture coordinates buffer initialization.
    EntityId tex_coord_buffer_id = NullId;
    if (!textures.empty())
    {
        auto maybe_tex_coord_buffer_id = CreateBufferInLevel(
            level, textures, std::format("{}.{}.texture", name, counter));
        if (maybe_tex_coord_buffer_id)
            tex_coord_buffer_id = maybe_tex_coord_buffer_id.value();
    }

    // Index buffer array.
    auto maybe_index_buffer_id = CreateBufferInLevel(
        level,
        indices,
        std::format("{}.{}.index", name, counter),
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
    if (!maybe_index_buffer_id)
        return {NullId, NullId};
    EntityId index_buffer_id = maybe_index_buffer_id.value();

    // Triangle buffer generation (SSBO).
    // Each triangle is composed of three vertices, each storing position,
    // normal, and texture coordinates. The vertex layout mirrors the GLSL
    // `Vertex` struct and occupies 48 bytes, yielding 144 bytes per triangle
    // (already aligned to 16 bytes).
    std::vector<float> triangles;
    std::vector<std::uint32_t> trace_indices(indices.begin(), indices.end());
    bool downsampled = false;
    {
        GLint max_ssbo_bytes = 0;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_bytes);
        if (max_ssbo_bytes > 0)
        {
            constexpr std::size_t kFloatsPerVertex = 12;
            constexpr std::size_t kBytesPerTriangle =
                kFloatsPerVertex * 3 * sizeof(float);
            const std::size_t triangle_count = trace_indices.size() / 3;
            const std::size_t max_triangles =
                static_cast<std::size_t>(max_ssbo_bytes) / kBytesPerTriangle;
            if (max_triangles > 0 && triangle_count > max_triangles)
            {
                const std::size_t step =
                    (triangle_count + max_triangles - 1) / max_triangles;
                std::vector<std::uint32_t> reduced;
                reduced.reserve(((triangle_count + step - 1) / step) * 3);
                for (std::size_t tri = 0; tri < triangle_count; tri += step)
                {
                    const std::size_t base = tri * 3;
                    reduced.push_back(trace_indices[base]);
                    reduced.push_back(trace_indices[base + 1]);
                    reduced.push_back(trace_indices[base + 2]);
                }
                Logger::GetInstance()->warn(
                    "Raytracing mesh {} has {} triangles; SSBO limit {} bytes. "
                    "Downsampling to {} triangles for GPU compatibility.",
                    name,
                    triangle_count,
                    static_cast<std::size_t>(max_ssbo_bytes),
                    reduced.size() / 3);
                trace_indices.swap(reduced);
                downsampled = true;
            }
        }
    }
    const bool build_bvh =
        acceleration_structure_enum == proto::NodeStaticMesh::BVH_ACCELERATION;
    auto push_vertex = [&](int idx) {
        // Position
        triangles.push_back(points[idx * 3]);
        triangles.push_back(points[idx * 3 + 1]);
        triangles.push_back(points[idx * 3 + 2]);
        triangles.push_back(0.0f); // Padding
        // Normal
        if (!normals.empty())
        {
            triangles.push_back(normals[idx * 3]);
            triangles.push_back(normals[idx * 3 + 1]);
            triangles.push_back(normals[idx * 3 + 2]);
        }
        else
        {
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
        }
        triangles.push_back(0.0f); // Padding
        // UV
        if (!textures.empty())
        {
            triangles.push_back(textures[idx * 2]);
            triangles.push_back(textures[idx * 2 + 1]);
        }
        else
        {
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
        }
        triangles.push_back(0.0f); // Padding
        triangles.push_back(0.0f); // Padding
    };

    EntityId bvh_buffer_id = NullId;
    if (build_bvh)
    {
        std::vector<frame::BVHNode> bvh_nodes;
        bool bvh_from_cache = false;
        const bool allow_cache = cache_metadata.has_value() && !downsampled;
        if (allow_cache)
        {
            auto cached = frame::LoadBvhCache(*cache_metadata);
            if (cached)
            {
                bvh_nodes = std::move(*cached);
                bvh_from_cache = true;
            }
        }
        if (!bvh_from_cache)
        {
            bvh_nodes = frame::BuildBVH(points, trace_indices);
            if (allow_cache)
            {
                frame::SaveBvhCache(*cache_metadata, bvh_nodes);
            }
        }
        auto maybe_bvh_buffer_id = CreateBufferInLevel(
            level,
            bvh_nodes,
            std::format("{}.{}.bvh", name, counter),
            opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER);
        if (!maybe_bvh_buffer_id)
            return {NullId, NullId};
        bvh_buffer_id = maybe_bvh_buffer_id.value();
    }

    for (std::size_t i = 0; i + 2 < trace_indices.size(); i += 3)
    {
        int i0 = static_cast<int>(trace_indices[i]);
        int i1 = static_cast<int>(trace_indices[i + 1]);
        int i2 = static_cast<int>(trace_indices[i + 2]);
        push_vertex(i0);
        push_vertex(i1);
        push_vertex(i2);
    }
    auto maybe_triangle_buffer_id = CreateBufferInLevel(
        level,
        triangles,
        std::format("{}.{}.triangle", name, counter),
        opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER);
    if (!maybe_triangle_buffer_id)
        return {NullId, NullId};
    EntityId triangle_buffer_id = maybe_triangle_buffer_id.value();

    StaticMeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.texture_buffer_id = tex_coord_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.triangle_buffer_id = triangle_buffer_id;
    parameter.bvh_buffer_id = bvh_buffer_id;
    auto static_mesh = std::make_unique<opengl::StaticMesh>(level, parameter);
    auto material_id = NullId;
    if (!material_ids.empty())
    {
        if (material_ids.size() != 1)
        {
            throw std::runtime_error("should only have 1 material here.");
        }
        material_id = material_ids[0];
    }
    // Mesh names must differ from node names, which are also based on the
    // original "name" parameter.  Otherwise, adding both the mesh and its
    // node to the level triggers a duplicate-name error.  Give the mesh a
    // distinct suffix so it can coexist with a node of the same base name.
    std::string mesh_name = std::format("{}.{}.mesh", name, counter);
    static_mesh->SetName(mesh_name);
    auto maybe_mesh_id = level.AddStaticMesh(std::move(static_mesh));
    if (!maybe_mesh_id)
        return {NullId, NullId};
    return {maybe_mesh_id, material_id};
}

EntityId LoadStaticMeshFromPly(
    LevelInterface& level, const frame::file::Ply& ply, const std::string& name)
{
    EntityId result = NullId;
    std::vector<float> points;
    std::vector<float> normals;
    std::vector<float> textures;
    std::vector<float> colors;
    for (const auto& point : ply.GetVertices())
    {
        points.push_back(point.x);
        points.push_back(point.y);
        points.push_back(point.z);
    }
    for (const auto& normal : ply.GetNormals())
    {
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }
    for (const auto& color : ply.GetColors())
    {
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    for (const auto& texcoord : ply.GetTextureCoordinates())
    {
        textures.push_back(texcoord.x);
        textures.push_back(texcoord.y);
    }
    const auto& indices = ply.GetIndices();

    // Point buffer initialization.
    auto maybe_point_buffer_id =
        CreateBufferInLevel(level, points, std::format("{}.point", name));
    if (!maybe_point_buffer_id)
        return NullId;
    EntityId point_buffer_id = maybe_point_buffer_id.value();

    // Color buffer initialization.
    EntityId color_buffer_id = NullId;
    if (!colors.empty())
    {
        auto maybe_color_buffer_id =
            CreateBufferInLevel(level, colors, std::format("{}.color", name));
        if (maybe_color_buffer_id)
            color_buffer_id = maybe_color_buffer_id.value();
    }

    // Normal buffer initialization.
    EntityId normal_buffer_id = NullId;
    if (!normals.empty())
    {
        auto maybe_normal_buffer_id =
            CreateBufferInLevel(level, normals, std::format("{}.normal", name));
        if (maybe_normal_buffer_id)
            normal_buffer_id = maybe_normal_buffer_id.value();
    }

    // Texture coordinates buffer initialization.
    EntityId tex_coord_buffer_id = NullId;
    if (!textures.empty())
    {
        auto maybe_tex_coord_buffer_id =
            CreateBufferInLevel(level, textures, std::format("{}.texture", name));
        if (maybe_tex_coord_buffer_id)
            tex_coord_buffer_id = maybe_tex_coord_buffer_id.value();
    }

    // Index buffer array.
    auto maybe_index_buffer_id = CreateBufferInLevel(
        level,
        indices,
        std::format("{}.index", name),
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
    if (!maybe_index_buffer_id)
        return NullId;
    EntityId index_buffer_id = maybe_index_buffer_id.value();

    std::unique_ptr<opengl::StaticMesh> static_mesh = nullptr;

    StaticMeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.index_buffer_id = index_buffer_id;

    // Add for present buffer.
    if (!normals.empty())
    {
        parameter.normal_buffer_id = normal_buffer_id;
    }
    if (!textures.empty())
    {
        parameter.texture_buffer_id = tex_coord_buffer_id;
    }
    if (!colors.empty())
    {
        parameter.color_buffer_id = color_buffer_id;
    }

    static_mesh = std::make_unique<opengl::StaticMesh>(level, parameter);
    // As with OBJ meshes, ensure the static mesh name is distinct from the
    // scene node name to avoid duplicate identifiers within the level.
    std::string mesh_name = std::format("{}.mesh", name);
    static_mesh->SetName(mesh_name);
    auto maybe_mesh_id = level.AddStaticMesh(std::move(static_mesh));
    if (!maybe_mesh_id)
        return NullId;
    return maybe_mesh_id;
}

std::vector<std::pair<EntityId, EntityId>> LoadStaticMeshesFromObjFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name /* = ""*/,
    proto::NodeStaticMesh::AccelerationStructureEnum acceleration_structure_enum)
{
    std::vector<std::pair<EntityId, EntityId>> entity_id_vec;
    std::filesystem::path final_path = ResolveAssetPath(file);
    frame::file::Obj obj(final_path);
    const auto& meshes = obj.GetMeshes();
    Logger& logger = Logger::GetInstance();
    std::vector<EntityId> material_ids;
    if (!material_name.empty())
    {
        auto maybe_id = level.GetIdFromName(material_name);
        if (maybe_id)
        {
            material_ids.push_back(maybe_id);
        }
    }
    else
    {
        for (const auto& material : obj.GetMaterials())
        {
            auto maybe_id = LoadMaterialFromObj(level, material);
            if (maybe_id)
            {
                material_ids.push_back(*maybe_id);
            }
        }
    }
    logger->info("Found in obj<{}> : {} meshes.", file.string(), meshes.size());

    const bool build_bvh =
        acceleration_structure_enum == proto::NodeStaticMesh::BVH_ACCELERATION;
    std::optional<frame::BvhCacheMetadata> base_cache_metadata;
    std::optional<std::filesystem::path> cache_root;
    std::filesystem::path asset_root;
    if (build_bvh)
    {
        std::error_code metadata_error;
        auto source_size = std::filesystem::file_size(final_path, metadata_error);
        if (!metadata_error)
        {
            auto write_time = std::filesystem::last_write_time(
                final_path, metadata_error);
            if (!metadata_error)
            {
                const auto mtime_ns =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        write_time.time_since_epoch())
                        .count();
                frame::BvhCacheMetadata metadata;
                metadata.source_relative = frame::file::PurifyFilePath(final_path);
                metadata.source_size = static_cast<std::uint64_t>(source_size);
                metadata.source_mtime_ns =
                    static_cast<std::uint64_t>(mtime_ns);
                try
                {
                    asset_root = frame::file::FindDirectory("asset");
                    cache_root = (asset_root / "cache").lexically_normal();
                    base_cache_metadata = metadata;
                }
                catch (const std::exception& exception)
                {
                    logger->warn(
                        "BVH cache disabled: {}", exception.what());
                }
            }
        }
        else
        {
            logger->info(
                "BVH cache disabled for {}: unable to inspect source file.",
                file.string());
        }
    }

    int mesh_counter = 0;
    for (const auto& mesh : meshes)
    {
        EntityId material_id{};
        if (!material_ids.empty())
        {
            if (material_ids.size() == 1)
            {
                material_id = material_ids[0];
            }
            else
            {
                if (mesh.GetMaterialId() < material_ids.size())
                {
                    material_id = material_ids[mesh.GetMaterialId()];
                }
            }
        }
        std::optional<frame::BvhCacheMetadata> cache_metadata;
        if (build_bvh && base_cache_metadata && cache_root)
        {
            frame::BvhCacheMetadata metadata = *base_cache_metadata;
            std::error_code relative_error;
            auto relative = std::filesystem::relative(final_path, asset_root, relative_error);
            if (relative_error)
            {
                relative = final_path.filename();
            }
            auto cache_dir = cache_root->lexically_normal();
            if (relative.has_parent_path() && relative.parent_path() != ".")
            {
                cache_dir /= relative.parent_path();
            }
            std::string stem = relative.stem().string();
            if (stem.empty())
            {
                stem = final_path.stem().string();
            }
            std::string cache_filename = std::format("{}-{}.bvhpb", stem, mesh_counter);
            for (auto& ch : cache_filename)
            {
                if (ch == ' ')
                {
                    ch = '_';
                }
            }
            metadata.cache_path = (cache_dir / cache_filename).lexically_normal();
            metadata.cache_relative = frame::file::PurifyFilePath(metadata.cache_path);
            cache_metadata = std::move(metadata);
        }
        auto [static_mesh_id, returned_material_id] = LoadStaticMeshFromObj(
            level,
            mesh,
            name,
            {material_id},
            mesh_counter,
            cache_metadata,
            acceleration_structure_enum);
        if (!static_mesh_id)
            return {};
        auto func = [&level](const std::string& name) -> NodeInterface* {
            auto maybe_id = level.GetIdFromName(name);
            if (!maybe_id)
            {
                throw std::runtime_error(
                    std::format("no id for name: {}", name));
            }
            return &level.GetSceneNodeFromId(maybe_id);
        };
        auto ptr = std::make_unique<NodeStaticMesh>(func, static_mesh_id);
        std::string node_name = meshes.size() == 1
                                    ? name
                                    : std::format("{}.{}", name, mesh_counter);
        ptr->SetName(node_name);
        auto maybe_id = level.AddSceneNode(std::move(ptr));
        if (!maybe_id)
        {
            return {};
        }
        entity_id_vec.push_back({maybe_id, returned_material_id});
        mesh_counter++;
    }
    return entity_id_vec;
}

std::pair<EntityId, EntityId> LoadStaticMeshFromPlyFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name /* = ""*/)
{
    EntityId entity_id = NullId;
    frame::file::Ply ply(file);
    Logger& logger = Logger::GetInstance();
    EntityId material_id = NullId;
    if (!material_name.empty())
    {
        auto maybe_id = level.GetIdFromName(material_name);
        if (maybe_id)
            material_id = maybe_id;
    }
    auto static_mesh_id = LoadStaticMeshFromPly(level, ply, name);
    if (!static_mesh_id)
        return {NullId, NullId};
    auto func = [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
        {
            throw std::runtime_error(std::format("no id for name: {}", name));
        }
        return &level.GetSceneNodeFromId(maybe_id);
    };
    auto ptr = std::make_unique<NodeStaticMesh>(func, static_mesh_id);
    ptr->SetName(name);
    auto maybe_id = level.AddSceneNode(std::move(ptr));
    if (!maybe_id)
        return {NullId, NullId};
    return {maybe_id, material_id};
}

} // End namespace.

std::vector<std::pair<EntityId, EntityId>> LoadStaticMeshesFromFile(
    LevelInterface& level_interface,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name)
{
    return LoadStaticMeshesFromFile(
        level_interface,
        std::move(file),
        name,
        material_name,
        proto::NodeStaticMesh::NO_ACCELERATION);
}

std::vector<std::pair<EntityId, EntityId>> LoadStaticMeshesFromFile(
    LevelInterface& level_interface,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name /* = ""*/,
    proto::NodeStaticMesh::AccelerationStructureEnum acceleration_structure_enum)
{
    auto extension = file.extension();
    std::filesystem::path final_path = ResolveAssetPath(file);
    if (extension == ".obj")
    {
        return LoadStaticMeshesFromObjFile(
            level_interface,
            final_path,
            name,
            material_name,
            acceleration_structure_enum);
    }
    if (extension == ".ply")
    {
        return {LoadStaticMeshFromPlyFile(
            level_interface, final_path, name, material_name)};
    }
    throw std::runtime_error(
        std::format("Unknown extention for file : {}", file.string()));
}

} // End namespace frame::opengl::file.
