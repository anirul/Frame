#include "frame/vulkan/raytrace_geometry_utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "frame/bvh.h"
#include "frame/json/program_key.h"
#include "frame/level.h"
#include "frame/node_mesh.h"
#include "frame/vulkan/buffer.h"

namespace frame::vulkan
{

float ReadTextureFirstChannel(const frame::TextureInterface& texture)
{
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT: {
        const auto data = texture.GetTextureFloat();
        return data.empty() ? 0.0f : data.front();
    }
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF: {
        const auto data = texture.GetTextureWord();
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 65535.0f;
    }
    case frame::proto::PixelElementSize::BYTE:
    default: {
        const auto data = texture.GetTextureByte();
        return data.empty() ? 0.0f : static_cast<float>(data.front()) / 255.0f;
    }
    }
}

std::array<float, 4> ReadTextureColor(const frame::TextureInterface& texture)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    const auto pixel_structure = texture.GetData().pixel_structure().value();
    std::size_t red_index = 0;
    std::size_t green_index = 0;
    std::size_t blue_index = 0;
    int alpha_index = -1;
    std::size_t channel_count = 1;
    switch (pixel_structure)
    {
    case frame::proto::PixelStructure::GREY:
        channel_count = 1;
        break;
    case frame::proto::PixelStructure::GREY_ALPHA:
        channel_count = 2;
        alpha_index = 1;
        break;
    case frame::proto::PixelStructure::RGB:
        channel_count = 3;
        break;
    case frame::proto::PixelStructure::RGB_ALPHA:
        channel_count = 4;
        alpha_index = 3;
        break;
    case frame::proto::PixelStructure::BGR:
        channel_count = 3;
        red_index = 2;
        blue_index = 0;
        break;
    case frame::proto::PixelStructure::BGR_ALPHA:
        channel_count = 4;
        red_index = 2;
        blue_index = 0;
        alpha_index = 3;
        break;
    default:
        return kWhite;
    }

    const auto build_color = [&](const auto& data, float scale) {
        if (data.size() < channel_count)
        {
            return kWhite;
        }
        const auto read_channel = [&](std::size_t index) {
            return static_cast<float>(data[index]) / scale;
        };
        const float red = read_channel(red_index);
        const float green = read_channel(green_index);
        const float blue = read_channel(blue_index);
        const float alpha =
            alpha_index >= 0
                ? read_channel(static_cast<std::size_t>(alpha_index))
                : 1.0f;
        return std::array<float, 4>{red, green, blue, alpha};
    };

    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT:
        return build_color(texture.GetTextureFloat(), 1.0f);
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF:
        return build_color(texture.GetTextureWord(), 65535.0f);
    case frame::proto::PixelElementSize::BYTE:
    default:
        return build_color(texture.GetTextureByte(), 255.0f);
    }
}

bool IsTransmissiveMaterial(
    frame::LevelInterface& level, frame::EntityId material_id)
{
    if (!material_id)
    {
        return false;
    }
    const auto& material = level.GetMaterialFromId(material_id);
    for (const auto texture_id : material.GetTextureIds())
    {
        if (material.GetInnerName(texture_id) != "transmission_texture")
        {
            continue;
        }
        return ReadTextureFirstChannel(level.GetTextureFromId(texture_id)) >
               0.01f;
    }
    return false;
}

frame::EntityId FindTextureIdByInnerName(
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto texture_id : material.GetTextureIds())
    {
        if (material.GetInnerName(texture_id) == expected_inner_name)
        {
            return texture_id;
        }
    }
    return frame::NullId;
}

bool IsRaytracingSourceMaterial(
    frame::LevelInterface& level, frame::EntityId material_id)
{
    if (material_id == frame::NullId)
    {
        return false;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = material.GetProgramId(&level);
    if (program_id == frame::NullId)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    if (!frame::json::IsRaytracingProgramKey(key))
    {
        return false;
    }
    return material.GetPreprocessProgramId(&level) != frame::NullId;
}

std::vector<std::pair<frame::EntityId, frame::EntityId>>
GetRaytracingSourceMeshMaterials(frame::LevelInterface& level)
{
    std::vector<std::pair<frame::EntityId, frame::EntityId>> pairs = {};
    const auto append_pairs =
        [&](frame::proto::NodeMesh::RenderTimeEnum render_time_enum) {
            for (const auto& pair : level.GetMeshMaterialIds(render_time_enum))
            {
                if (IsRaytracingSourceMaterial(level, pair.second))
                {
                    pairs.push_back(pair);
                }
            }
        };
    append_pairs(frame::proto::NodeMesh::PRE_RENDER_TIME);
    append_pairs(frame::proto::NodeMesh::SCENE_RENDER_TIME);
    return pairs;
}

std::array<float, 4> ResolveRaytracingSourceMaterialColor(
    frame::LevelInterface& level, frame::EntityId material_id)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!material_id)
    {
        return kWhite;
    }

    const auto& material = level.GetMaterialFromId(material_id);
    auto color_texture_id =
        FindTextureIdByInnerName(material, "albedo_texture");
    if (!color_texture_id)
    {
        color_texture_id = FindTextureIdByInnerName(material, "Color");
    }
    if (!color_texture_id)
    {
        return kWhite;
    }
    return ReadTextureColor(level.GetTextureFromId(color_texture_id));
}

std::array<float, 4> ResolveRaytracingReferenceColor(
    frame::LevelInterface& level, bool transmissive)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)source_node_id;
        if (IsTransmissiveMaterial(level, source_material_id) == transmissive)
        {
            return ResolveRaytracingSourceMaterialColor(
                level, source_material_id);
        }
    }
    return kWhite;
}

std::array<float, 4> ResolveRaytracingColorMultiplier(
    const std::array<float, 4>& source_color,
    const std::array<float, 4>& reference_color)
{
    std::array<float, 4> multiplier = {1.0f, 1.0f, 1.0f, source_color[3]};
    for (std::size_t channel = 0; channel < 3; ++channel)
    {
        float value = source_color[channel];
        if (reference_color[channel] > 0.0001f)
        {
            value /= reference_color[channel];
        }
        multiplier[channel] = std::clamp(value, 0.0f, 4.0f);
    }
    return multiplier;
}

struct RaytracingTextureMapping
{
    const char* source_name;
    const char* fallback_name;
    const char* target_name;
};

constexpr std::array<RaytracingTextureMapping, 7>
    kOpaqueRaytracingTextureMappings = {{
        {"albedo_texture", "Color", "opaque_albedo_texture"},
        {"normal_texture", nullptr, "opaque_normal_texture"},
        {"roughness_texture", nullptr, "opaque_roughness_texture"},
        {"metallic_texture", nullptr, "opaque_metallic_texture"},
        {"ao_texture", nullptr, "opaque_ao_texture"},
        {"specular_factor_texture", nullptr, "opaque_specular_factor_texture"},
        {"specular_color_texture", nullptr, "opaque_specular_color_texture"},
    }};

constexpr std::array<RaytracingTextureMapping, 10>
    kTransmissiveRaytracingTextureMappings = {{
        {"albedo_texture", "Color", "transmissive_albedo_texture"},
        {"normal_texture", nullptr, "transmissive_normal_texture"},
        {"roughness_texture", nullptr, "transmissive_roughness_texture"},
        {"metallic_texture", nullptr, "transmissive_metallic_texture"},
        {"ao_texture", nullptr, "transmissive_ao_texture"},
        {"transmission_texture", nullptr, "transmissive_transmission_texture"},
        {"ior_texture", nullptr, "transmissive_ior_texture"},
        {"thickness_texture", nullptr, "transmissive_thickness_texture"},
        {"attenuation_color_texture",
         nullptr,
         "transmissive_attenuation_color_texture"},
        {"attenuation_distance_texture",
         nullptr,
         "transmissive_attenuation_distance_texture"},
    }};

struct RaytracingTextureAtlasLayout
{
    std::vector<frame::EntityId> material_ids = {};
    std::unordered_map<frame::EntityId, std::uint32_t> slot_by_material_id = {};
    std::uint32_t columns = 1u;
    std::uint32_t rows = 1u;
    std::uint32_t tile_width = 1u;
    std::uint32_t tile_height = 1u;
    std::uint32_t atlas_width = 1u;
    std::uint32_t atlas_height = 1u;
};

frame::EntityId ResolveRaytracingMappedTextureId(
    const frame::MaterialInterface& material,
    const RaytracingTextureMapping& mapping)
{
    auto texture_id = FindTextureIdByInnerName(material, mapping.source_name);
    if (!texture_id && mapping.fallback_name)
    {
        texture_id = FindTextureIdByInnerName(material, mapping.fallback_name);
    }
    return texture_id;
}

template <typename MappingArray>
std::optional<RaytracingTextureAtlasLayout> BuildRaytracingTextureAtlasLayout(
    frame::LevelInterface& level,
    bool transmissive,
    const MappingArray& mappings)
{
    std::vector<frame::EntityId> material_ids = {};
    std::unordered_set<frame::EntityId> seen_material_ids = {};
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)source_node_id;
        if (IsTransmissiveMaterial(level, source_material_id) != transmissive ||
            !source_material_id ||
            !seen_material_ids.insert(source_material_id).second)
        {
            continue;
        }
        material_ids.push_back(source_material_id);
    }
    if (material_ids.size() <= 1u)
    {
        return std::nullopt;
    }

    RaytracingTextureAtlasLayout layout = {};
    layout.material_ids = std::move(material_ids);
    for (std::size_t i = 0; i < layout.material_ids.size(); ++i)
    {
        layout.slot_by_material_id.emplace(
            layout.material_ids[i], static_cast<std::uint32_t>(i));
    }

    for (const auto material_id : layout.material_ids)
    {
        const auto& material = level.GetMaterialFromId(material_id);
        for (const auto& mapping : mappings)
        {
            const auto texture_id =
                ResolveRaytracingMappedTextureId(material, mapping);
            if (!texture_id)
            {
                continue;
            }
            const auto size = level.GetTextureFromId(texture_id).GetSize();
            layout.tile_width =
                std::max(layout.tile_width, std::max(1u, size.x));
            layout.tile_height =
                std::max(layout.tile_height, std::max(1u, size.y));
        }
    }

    const auto material_count = static_cast<double>(layout.material_ids.size());
    layout.columns = std::max(
        1u, static_cast<std::uint32_t>(std::ceil(std::sqrt(material_count))));
    layout.rows = std::max(
        1u,
        static_cast<std::uint32_t>(
            (layout.material_ids.size() + layout.columns - 1u) /
            layout.columns));
    layout.atlas_width = std::max(1u, layout.columns * layout.tile_width);
    layout.atlas_height = std::max(1u, layout.rows * layout.tile_height);
    return layout;
}

bool HasBoundRaytracingTextureAtlas(
    frame::LevelInterface& level, bool transmissive)
{
    const auto scene_material_id = level.GetIdFromName("RayTraceMaterial");
    if (scene_material_id == frame::NullId)
    {
        return false;
    }
    const auto& scene_material = level.GetMaterialFromId(scene_material_id);
    const auto* target_name =
        transmissive ? "transmissive_albedo_texture" : "opaque_albedo_texture";
    const auto texture_id =
        FindTextureIdByInnerName(scene_material, target_name);
    if (texture_id == frame::NullId)
    {
        return false;
    }
    return level.GetNameFromId(texture_id).find(".__raytrace_atlas_") !=
           std::string::npos;
}

template <typename T> void HashCombine(std::size_t& seed, const T& value)
{
    seed ^= std::hash<T>{}(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
}

void HashFloat(std::size_t& seed, float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    HashCombine(seed, bits);
}

void HashColor(std::size_t& seed, const std::array<float, 4>& color)
{
    for (const float channel : color)
    {
        HashFloat(seed, channel);
    }
}

void HashMatrix(std::size_t& seed, const glm::mat4& matrix)
{
    const float* values = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i)
    {
        HashFloat(seed, values[i]);
    }
}

void HashByteSamples(std::size_t& seed, const std::vector<std::uint8_t>& bytes)
{
    HashCombine(seed, bytes.size());
    if (bytes.empty())
    {
        return;
    }

    constexpr std::array<std::pair<std::size_t, std::size_t>, 8> kRatios = {{
        {0u, 1u},
        {11u, 100u},
        {23u, 100u},
        {37u, 100u},
        {53u, 100u},
        {67u, 100u},
        {83u, 100u},
        {1u, 1u},
    }};
    const std::size_t max_index = bytes.size() - 1;
    for (const auto& [numerator, denominator] : kRatios)
    {
        const auto index = (max_index * numerator) / denominator;
        HashCombine(seed, bytes[index]);
    }
}

std::size_t BuildRaytracingSourceStateHash(
    frame::LevelInterface& level,
    double time_seconds,
    bool include_node_matrices)
{
    std::size_t state_hash = 0;
    HashCombine(state_hash, include_node_matrices);
    const auto source_mesh_materials = GetRaytracingSourceMeshMaterials(level);
    HashCombine(state_hash, source_mesh_materials.size());
    for (const auto& [source_node_id, source_material_id] :
         source_mesh_materials)
    {
        HashCombine(state_hash, static_cast<std::uint64_t>(source_node_id));
        HashCombine(state_hash, static_cast<std::uint64_t>(source_material_id));

        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            continue;
        }

        if (include_node_matrices)
        {
            HashMatrix(state_hash, node->GetLocalModel(time_seconds));
        }
        HashCombine(
            state_hash, IsTransmissiveMaterial(level, source_material_id));
        HashColor(
            state_hash,
            ResolveRaytracingSourceMaterialColor(level, source_material_id));

        const auto mesh_id = node->GetLocalMesh();
        HashCombine(state_hash, static_cast<std::uint64_t>(mesh_id));
        if (!mesh_id)
        {
            continue;
        }

        const auto triangle_buffer_id =
            level.GetMeshFromId(mesh_id).GetTriangleBufferId();
        HashCombine(state_hash, static_cast<std::uint64_t>(triangle_buffer_id));
        if (!triangle_buffer_id)
        {
            continue;
        }

        auto* triangle_buffer =
            dynamic_cast<Buffer*>(&level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        HashByteSamples(state_hash, triangle_buffer->GetRawData());
    }
    return state_hash;
}

bool RaytraceSceneRequiresWorldSpaceBuffers(frame::LevelInterface& level)
{
    // Vulkan raytracing paths can keep dynamic skinned meshes in object space
    // and update BLAS/TLAS data directly. Software BVH builds still request
    // world-space triangles separately via the build_software_bvh flag.
    (void)level;
    return false;
}

bool HasRaytracingSourceMeshes(frame::LevelInterface& level)
{
    return !GetRaytracingSourceMeshMaterials(level).empty();
}

std::optional<glm::mat4> GetSharedRaytraceSceneTransform(
    frame::LevelInterface& level, double time_seconds)
{
    const auto source_mesh_materials = GetRaytracingSourceMeshMaterials(level);
    if (source_mesh_materials.empty())
    {
        return std::nullopt;
    }

    std::optional<glm::mat4> shared_model = std::nullopt;
    for (const auto& [source_node_id, source_material_id] :
         source_mesh_materials)
    {
        (void)source_material_id;
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            return std::nullopt;
        }
        const glm::mat4 model = node->GetLocalModel(time_seconds);
        if (!shared_model)
        {
            shared_model = model;
            continue;
        }
        if (*shared_model != model)
        {
            return std::nullopt;
        }
    }
    return shared_model;
}

bool CanUseSharedTransformHardwareRaytraceScene(
    frame::LevelInterface& level, double time_seconds)
{
    return !RaytraceSceneRequiresWorldSpaceBuffers(level) &&
           GetSharedRaytraceSceneTransform(level, time_seconds).has_value();
}

std::array<float, 4> GetRaytracingAtlasUvBounds(
    const RaytracingTextureAtlasLayout& layout, frame::EntityId material_id)
{
    const auto it = layout.slot_by_material_id.find(material_id);
    if (it == layout.slot_by_material_id.end())
    {
        return {0.0f, 1.0f, 0.0f, 1.0f};
    }

    const std::uint32_t slot = it->second;
    const std::uint32_t column = slot % layout.columns;
    const std::uint32_t row = slot / layout.columns;
    const float atlas_width = static_cast<float>(layout.atlas_width);
    const float atlas_height = static_cast<float>(layout.atlas_height);
    const float u0 =
        (static_cast<float>(column * layout.tile_width) + 0.5f) / atlas_width;
    const float u1 =
        (static_cast<float>((column + 1u) * layout.tile_width) - 0.5f) /
        atlas_width;
    const float v0 =
        (static_cast<float>(row * layout.tile_height) + 0.5f) / atlas_height;
    const float v1 =
        (static_cast<float>((row + 1u) * layout.tile_height) - 0.5f) /
        atlas_height;
    return {u0, std::max(u0, u1), v0, std::max(v0, v1)};
}

float WrapRaytracingAtlasUv(float value)
{
    if (!std::isfinite(value))
    {
        return 0.0f;
    }
    return value - std::floor(value);
}

std::vector<std::uint8_t> RemapRaytracingAtlasTriangleUvs(
    const std::vector<std::uint8_t>& raw,
    const std::optional<RaytracingTextureAtlasLayout>& layout,
    frame::EntityId material_id)
{
    if (!layout || raw.empty() ||
        !layout->slot_by_material_id.contains(material_id))
    {
        return raw;
    }
    if (raw.size() % sizeof(RaytraceVertex) != 0u)
    {
        throw std::runtime_error(
            "Vulkan raytrace triangle buffer size is not aligned to the "
            "expected vertex stride.");
    }

    const auto bounds = GetRaytracingAtlasUvBounds(*layout, material_id);
    std::vector<std::uint8_t> remapped = raw;
    auto* vertices = reinterpret_cast<RaytraceVertex*>(remapped.data());
    const std::size_t vertex_count = remapped.size() / sizeof(RaytraceVertex);
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        const float u = WrapRaytracingAtlasUv(vertices[i].u);
        const float v = WrapRaytracingAtlasUv(vertices[i].v);
        vertices[i].u = bounds[0] + (bounds[1] - bounds[0]) * u;
        vertices[i].v = bounds[2] + (bounds[3] - bounds[2]) * v;
    }
    return remapped;
}

struct alignas(16) GpuSkinningSourceVertex
{
    glm::vec4 position = glm::vec4(0.0f);
    glm::vec4 normal = glm::vec4(0.0f);
    glm::vec4 uv = glm::vec4(0.0f);
    glm::uvec4 bone_indices = glm::uvec4(0u);
    glm::vec4 bone_weights = glm::vec4(0.0f);
};

std::vector<std::uint8_t> BuildGpuSkinningSourceVertexBytes(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::int32_t>& bone_indices,
    const std::vector<float>& bone_weights)
{
    const std::size_t vertex_count = points.size() / 3u;
    std::vector<GpuSkinningSourceVertex> vertices(vertex_count);
    for (std::size_t vertex = 0; vertex < vertex_count; ++vertex)
    {
        const std::size_t point_offset = vertex * 3u;
        const std::size_t uv_offset = vertex * 2u;
        const std::size_t bone_offset = vertex * 4u;

        auto& dst = vertices[vertex];
        dst.position = glm::vec4(
            points[point_offset + 0u],
            points[point_offset + 1u],
            points[point_offset + 2u],
            1.0f);
        if (point_offset + 2u < normals.size())
        {
            dst.normal = glm::vec4(
                normals[point_offset + 0u],
                normals[point_offset + 1u],
                normals[point_offset + 2u],
                0.0f);
        }
        if (uv_offset + 1u < textures.size())
        {
            dst.uv = glm::vec4(
                textures[uv_offset + 0u], textures[uv_offset + 1u], 0.0f, 0.0f);
        }
        if (bone_offset + 3u < bone_indices.size())
        {
            dst.bone_indices = glm::uvec4(
                static_cast<std::uint32_t>(
                    std::max<std::int32_t>(bone_indices[bone_offset + 0u], 0)),
                static_cast<std::uint32_t>(
                    std::max<std::int32_t>(bone_indices[bone_offset + 1u], 0)),
                static_cast<std::uint32_t>(
                    std::max<std::int32_t>(bone_indices[bone_offset + 2u], 0)),
                static_cast<std::uint32_t>(
                    std::max<std::int32_t>(bone_indices[bone_offset + 3u], 0)));
        }
        if (bone_offset + 3u < bone_weights.size())
        {
            dst.bone_weights = glm::vec4(
                bone_weights[bone_offset + 0u],
                bone_weights[bone_offset + 1u],
                bone_weights[bone_offset + 2u],
                bone_weights[bone_offset + 3u]);
        }
    }

    std::vector<std::uint8_t> bytes(
        vertices.size() * sizeof(GpuSkinningSourceVertex));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), vertices.data(), bytes.size());
    }
    return bytes;
}

std::vector<std::uint8_t> BuildFlatBytes(
    const std::vector<std::uint32_t>& values)
{
    std::vector<std::uint8_t> bytes(values.size() * sizeof(std::uint32_t));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), values.data(), bytes.size());
    }
    return bytes;
}

std::vector<std::uint8_t> ApplyTriangleColorMultiplier(
    const std::vector<std::uint8_t>& raw, const std::array<float, 4>& color)
{
    if (raw.empty())
    {
        return raw;
    }
    if (raw.size() % sizeof(RaytraceVertex) != 0)
    {
        throw std::runtime_error(
            "Animated Vulkan raytrace triangle buffer size is not aligned to "
            "the expected vertex stride.");
    }

    std::vector<std::uint8_t> tinted = raw;
    auto* vertices = reinterpret_cast<RaytraceVertex*>(tinted.data());
    const std::size_t vertex_count = tinted.size() / sizeof(RaytraceVertex);
    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        vertices[i].pad0 = color[0];
        vertices[i].pad1 = color[1];
        vertices[i].pad2 = color[2];
        vertices[i].pad3 = color[3];
    }
    return tinted;
}

std::vector<std::uint8_t> TransformTriangleBytes(
    const std::vector<std::uint8_t>& raw, const glm::mat4& model)
{
    if (raw.empty() || model == glm::mat4(1.0f))
    {
        return raw;
    }
    if (raw.size() % sizeof(RaytraceVertex) != 0)
    {
        throw std::runtime_error(
            "Animated Vulkan raytrace triangle buffer size is not aligned to "
            "the expected vertex stride.");
    }

    std::vector<std::uint8_t> transformed = raw;
    auto* vertices = reinterpret_cast<RaytraceVertex*>(transformed.data());
    const std::size_t vertex_count =
        transformed.size() / sizeof(RaytraceVertex);

    glm::mat3 normal_matrix = glm::mat3(1.0f);
    const float det = glm::determinant(glm::mat3(model));
    if (std::abs(det) > 1.0e-8f)
    {
        normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
    }

    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        const glm::vec3 position = glm::vec3(
            model *
            glm::vec4(vertices[i].px, vertices[i].py, vertices[i].pz, 1.0f));
        vertices[i].px = position.x;
        vertices[i].py = position.y;
        vertices[i].pz = position.z;

        glm::vec3 normal =
            normal_matrix *
            glm::vec3(vertices[i].nx, vertices[i].ny, vertices[i].nz);
        if (glm::length(normal) > 1.0e-6f)
        {
            normal = glm::normalize(normal);
        }
        vertices[i].nx = normal.x;
        vertices[i].ny = normal.y;
        vertices[i].nz = normal.z;
    }
    return transformed;
}

std::optional<RaytracingSourceGeometryData>
BuildRaytracingSourceGeometryDataForSource(
    frame::LevelInterface& level,
    frame::EntityId source_node_id,
    frame::EntityId source_material_id,
    double time_seconds,
    bool apply_node_transform,
    std::uint32_t triangle_offset)
{
    const auto transmissive = IsTransmissiveMaterial(level, source_material_id);
    const auto reference_color =
        ResolveRaytracingReferenceColor(level, transmissive);

    auto* node = dynamic_cast<frame::NodeMesh*>(
        &level.GetSceneNodeFromId(source_node_id));
    if (!node)
    {
        return std::nullopt;
    }
    const auto mesh_id = node->GetLocalMesh();
    if (!mesh_id)
    {
        return std::nullopt;
    }

    const auto& mesh = level.GetMeshFromId(mesh_id);
    const auto triangle_buffer_id = mesh.GetTriangleBufferId();
    if (!triangle_buffer_id)
    {
        return std::nullopt;
    }

    auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
        &level.GetBufferFromId(triangle_buffer_id));
    if (!triangle_buffer)
    {
        return std::nullopt;
    }

    const auto source_color =
        ResolveRaytracingSourceMaterialColor(level, source_material_id);
    const auto color_multiplier =
        ResolveRaytracingColorMultiplier(source_color, reference_color);
    const auto atlas_layout =
        HasBoundRaytracingTextureAtlas(level, transmissive)
            ? (transmissive
                   ? BuildRaytracingTextureAtlasLayout(
                         level, true, kTransmissiveRaytracingTextureMappings)
                   : BuildRaytracingTextureAtlasLayout(
                         level, false, kOpaqueRaytracingTextureMappings))
            : std::nullopt;
    const auto remapped = RemapRaytracingAtlasTriangleUvs(
        triangle_buffer->GetRawData(), atlas_layout, source_material_id);
    const auto transformed =
        apply_node_transform ? TransformTriangleBytes(
                                   remapped, node->GetLocalModel(time_seconds))
                             : remapped;

    RaytracingSourceGeometryData geometry = {};
    geometry.source_node_id = source_node_id;
    geometry.triangle_buffer_id = triangle_buffer_id;
    geometry.source_material_id = source_material_id;
    geometry.material_id = transmissive ? 0u : 1u;
    geometry.triangle_offset = triangle_offset;
    geometry.atlas_uv_bounds =
        transmissive && atlas_layout
            ? GetRaytracingAtlasUvBounds(*atlas_layout, source_material_id)
        : (!transmissive && atlas_layout)
            ? GetRaytracingAtlasUvBounds(*atlas_layout, source_material_id)
            : std::array<float, 4>{0.0f, 1.0f, 0.0f, 1.0f};
    geometry.triangle_bytes =
        ApplyTriangleColorMultiplier(transformed, color_multiplier);
    return geometry;
}

std::vector<RaytracingSourceGeometryData> BuildRaytracingSourceGeometryData(
    frame::LevelInterface& level,
    double time_seconds,
    bool apply_node_transform)
{
    std::vector<RaytracingSourceGeometryData> geometries = {};
    std::uint32_t next_transmissive_triangle_offset = 0;
    std::uint32_t next_opaque_triangle_offset = 0;
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        const bool transmissive =
            IsTransmissiveMaterial(level, source_material_id);
        const std::uint32_t triangle_offset =
            transmissive ? next_transmissive_triangle_offset
                         : next_opaque_triangle_offset;
        auto geometry = BuildRaytracingSourceGeometryDataForSource(
            level,
            source_node_id,
            source_material_id,
            time_seconds,
            apply_node_transform,
            triangle_offset);
        if (!geometry)
        {
            continue;
        }
        const std::uint32_t triangle_count = static_cast<std::uint32_t>(
            geometry->triangle_bytes.size() /
            (kRaytraceTriangleVertexStrideBytes * 3u));
        if (transmissive)
        {
            next_transmissive_triangle_offset += triangle_count;
        }
        else
        {
            next_opaque_triangle_offset += triangle_count;
        }
        geometries.push_back(std::move(*geometry));
    }
    return geometries;
}

const RaytracingSourceGeometryData* FindPreparedRaytracingSourceGeometry(
    const std::vector<RaytracingSourceGeometryData>& prepared_source_geometries,
    frame::EntityId source_node_id,
    frame::EntityId source_material_id,
    std::uint32_t triangle_offset)
{
    for (const auto& source_geometry : prepared_source_geometries)
    {
        if (source_geometry.source_node_id == source_node_id &&
            source_geometry.source_material_id == source_material_id &&
            source_geometry.triangle_offset == triangle_offset)
        {
            return &source_geometry;
        }
    }
    return nullptr;
}

std::vector<RaytracingSourceGeometryData>
BuildPreparedUpdatedRaytracingSourceGeometries(
    frame::LevelInterface& level,
    const std::vector<EntityId>& updated_source_triangle_buffer_ids,
    double time_seconds)
{
    if (updated_source_triangle_buffer_ids.empty())
    {
        return {};
    }

    const auto was_updated = [&](EntityId buffer_id) {
        return std::find(
                   updated_source_triangle_buffer_ids.begin(),
                   updated_source_triangle_buffer_ids.end(),
                   buffer_id) != updated_source_triangle_buffer_ids.end();
    };

    std::vector<RaytracingSourceGeometryData> prepared_source_geometries = {};
    prepared_source_geometries.reserve(
        updated_source_triangle_buffer_ids.size());
    std::uint32_t next_transmissive_triangle_offset = 0;
    std::uint32_t next_opaque_triangle_offset = 0;
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        const bool transmissive =
            IsTransmissiveMaterial(level, source_material_id);
        const std::uint32_t triangle_offset =
            transmissive ? next_transmissive_triangle_offset
                         : next_opaque_triangle_offset;

        auto* node = dynamic_cast<frame::NodeMesh*>(
            &level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }

        const auto triangle_buffer_id =
            level.GetMeshFromId(mesh_id).GetTriangleBufferId();
        if (!triangle_buffer_id)
        {
            continue;
        }

        auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        const std::uint32_t triangle_count = static_cast<std::uint32_t>(
            triangle_buffer->GetRawData().size() /
            (kRaytraceTriangleVertexStrideBytes * 3u));
        if (was_updated(triangle_buffer_id))
        {
            auto source_geometry = BuildRaytracingSourceGeometryDataForSource(
                level,
                source_node_id,
                source_material_id,
                time_seconds,
                false,
                triangle_offset);
            if (source_geometry)
            {
                prepared_source_geometries.push_back(
                    std::move(*source_geometry));
            }
        }

        if (transmissive)
        {
            next_transmissive_triangle_offset += triangle_count;
        }
        else
        {
            next_opaque_triangle_offset += triangle_count;
        }
    }

    return prepared_source_geometries;
}

std::vector<std::uint8_t> BuildSequentialIndexBytes(std::uint32_t vertex_count)
{
    std::vector<std::uint32_t> indices(vertex_count);
    std::iota(indices.begin(), indices.end(), 0u);
    std::vector<std::uint8_t> bytes(indices.size() * sizeof(std::uint32_t));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), indices.data(), bytes.size());
    }
    return bytes;
}

std::vector<std::uint8_t> BuildAggregateTriangleBytes(
    frame::LevelInterface& level,
    bool transmissive,
    double time_seconds,
    bool apply_node_transform)
{
    std::vector<std::uint8_t> aggregate_triangle_bytes = {};
    const auto reference_color =
        ResolveRaytracingReferenceColor(level, transmissive);
    const auto atlas_layout =
        HasBoundRaytracingTextureAtlas(level, transmissive)
            ? (transmissive
                   ? BuildRaytracingTextureAtlasLayout(
                         level, true, kTransmissiveRaytracingTextureMappings)
                   : BuildRaytracingTextureAtlasLayout(
                         level, false, kOpaqueRaytracingTextureMappings))
            : std::nullopt;
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        if (IsTransmissiveMaterial(level, source_material_id) != transmissive)
        {
            continue;
        }

        auto* node = dynamic_cast<frame::NodeMesh*>(
            &level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }

        const auto& mesh = level.GetMeshFromId(mesh_id);
        const auto triangle_buffer_id = mesh.GetTriangleBufferId();
        if (!triangle_buffer_id)
        {
            continue;
        }

        auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        const auto source_color =
            ResolveRaytracingSourceMaterialColor(level, source_material_id);
        const auto color_multiplier =
            ResolveRaytracingColorMultiplier(source_color, reference_color);
        const auto remapped = RemapRaytracingAtlasTriangleUvs(
            triangle_buffer->GetRawData(), atlas_layout, source_material_id);
        const auto transformed =
            apply_node_transform
                ? TransformTriangleBytes(
                      remapped, node->GetLocalModel(time_seconds))
                : remapped;
        const auto tinted =
            ApplyTriangleColorMultiplier(transformed, color_multiplier);
        aggregate_triangle_bytes.insert(
            aggregate_triangle_bytes.end(), tinted.begin(), tinted.end());
    }
    return aggregate_triangle_bytes;
}

std::vector<std::uint8_t> BuildAggregateBvhBytes(
    const std::vector<std::uint8_t>& triangle_bytes)
{
    if (triangle_bytes.empty())
    {
        return {};
    }
    if (triangle_bytes.size() % kRaytraceTriangleVertexStrideBytes != 0)
    {
        throw std::runtime_error(
            "Animated raytrace triangle buffer size is not aligned to the "
            "expected vertex stride.");
    }

    const auto* triangle_floats =
        reinterpret_cast<const float*>(triangle_bytes.data());
    const std::size_t vertex_count =
        triangle_bytes.size() / kRaytraceTriangleVertexStrideBytes;
    std::vector<float> points = {};
    points.reserve(vertex_count * 3);
    for (std::size_t vertex_index = 0; vertex_index < vertex_count;
         ++vertex_index)
    {
        const std::size_t base = vertex_index * kRaytraceFloatsPerVertex;
        points.push_back(triangle_floats[base + 0]);
        points.push_back(triangle_floats[base + 1]);
        points.push_back(triangle_floats[base + 2]);
    }

    std::vector<std::uint32_t> indices(vertex_count);
    std::iota(indices.begin(), indices.end(), 0u);
    const auto bvh_nodes = frame::BuildBVH(points, indices);

    std::vector<std::uint8_t> bytes(bvh_nodes.size() * sizeof(frame::BVHNode));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), bvh_nodes.data(), bytes.size());
    }
    return bytes;
}

} // namespace frame::vulkan
