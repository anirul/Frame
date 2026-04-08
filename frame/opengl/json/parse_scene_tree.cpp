#include "frame/opengl/json/parse_scene_tree.h"

#include <array>
#include <cctype>
#include <cstring>
#include <format>
#include <optional>
#include <unordered_set>

#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/program_key.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/opengl/json/parse_texture.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/file/load_mesh.h"
#include "frame/opengl/material.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/mesh.h"

namespace frame::json
{

namespace
{

std::function<NodeInterface*(const std::string& name)> GetFunctor(
    LevelInterface& level)
{
    return [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
        {
            throw std::runtime_error(std::format("No id from name: {}", name));
        }
        EntityId id = maybe_id;
        return &level.GetSceneNodeFromId(id);
    };
}

bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        const auto l = static_cast<unsigned char>(lhs[i]);
        const auto r = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(l) != std::tolower(r))
        {
            return false;
        }
    }
    return true;
}

std::string ToLowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::optional<std::string> ResolveSamplerNameForTexture(
    const ProgramInterface& program,
    const std::string& texture_name)
{
    std::vector<std::string> sampler_names = {};
    for (const auto& binding : program.GetData().bindings())
    {
        if (binding.binding_type() ==
            frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER)
        {
            sampler_names.push_back(binding.name());
        }
    }
    if (!sampler_names.empty())
    {
        for (const auto& sampler_name : sampler_names)
        {
            if (sampler_name == texture_name)
            {
                return sampler_name;
            }
        }
        for (const auto& sampler_name : sampler_names)
        {
            if (EqualsIgnoreCase(sampler_name, texture_name))
            {
                return sampler_name;
            }
        }
        if (sampler_names.size() == 1)
        {
            return sampler_names.front();
        }
    }
    if (program.HasUniform(texture_name))
    {
        return texture_name;
    }
    return std::nullopt;
}

EntityId FindTextureIdByName(
    LevelInterface& level, const std::string& texture_name)
{
    for (const auto texture_id : level.GetTextures())
    {
        const auto candidate_name = level.GetNameFromId(texture_id);
        if (candidate_name == texture_name ||
            EqualsIgnoreCase(candidate_name, texture_name))
        {
            return texture_id;
        }
    }
    return NullId;
}

float ReadTextureFirstChannel(LevelInterface& level, EntityId texture_id)
{
    if (!texture_id)
    {
        return 0.0f;
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    const auto element_size = texture.GetData().pixel_element_size().value();
    switch (element_size)
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
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 255.0f;
    }
    }
}

std::array<float, 4> ReadTextureColor(LevelInterface& level, EntityId texture_id)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!texture_id)
    {
        return kWhite;
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    const auto pixel_structure = texture.GetData().pixel_structure().value();
    std::size_t red_index = 0;
    std::size_t green_index = 0;
    std::size_t blue_index = 0;
    std::optional<std::size_t> alpha_index = std::nullopt;
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
        red_index = 0;
        green_index = 1;
        blue_index = 2;
        break;
    case frame::proto::PixelStructure::RGB_ALPHA:
        channel_count = 4;
        red_index = 0;
        green_index = 1;
        blue_index = 2;
        alpha_index = 3;
        break;
    case frame::proto::PixelStructure::BGR:
        channel_count = 3;
        red_index = 2;
        green_index = 1;
        blue_index = 0;
        break;
    case frame::proto::PixelStructure::BGR_ALPHA:
        channel_count = 4;
        red_index = 2;
        green_index = 1;
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
        const float alpha = alpha_index ? read_channel(*alpha_index) : 1.0f;
        return std::array<float, 4>{red, green, blue, alpha};
    };

    const auto element_size = texture.GetData().pixel_element_size().value();
    switch (element_size)
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

void BindMaterialTexturesFromProgram(
    MaterialInterface& material,
    LevelInterface& level,
    const ProgramInterface& program)
{
    if (!material.GetTextureIds().empty())
    {
        return;
    }
    std::unordered_set<EntityId> bound_texture_ids = {};
    for (const auto texture_id : program.GetInputTextureIds())
    {
        if (!texture_id || !bound_texture_ids.insert(texture_id).second)
        {
            continue;
        }
        const auto texture_name = level.GetNameFromId(texture_id);
        const auto inner_name = ResolveSamplerNameForTexture(
                                    program,
                                    texture_name)
                                    .value_or(texture_name);
        material.AddTextureId(texture_id, inner_name);
    }
    for (const auto& binding : program.GetData().bindings())
    {
        if (binding.binding_type() !=
            frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER)
        {
            continue;
        }
        const auto texture_id = FindTextureIdByName(level, binding.name());
        if (!texture_id || !bound_texture_ids.insert(texture_id).second)
        {
            continue;
        }
        material.AddTextureId(texture_id, binding.name());
    }
}

void ConfigureMaterialProgramsForRenderTime(
    LevelInterface& level,
    EntityId material_id,
    proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    if (!material_id)
    {
        return;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (program_id)
    {
        material.SetProgramId(program_id);
        auto& program = level.GetProgramFromId(program_id);
        BindMaterialTexturesFromProgram(material, level, program);
    }
    const auto preprocess_program_id =
        level.GetRenderPassPreprocessProgramId(render_time_enum);
    if (preprocess_program_id)
    {
        material.SetPreprocessProgramId(preprocess_program_id);
    }
}

std::string GetAutoMaterialName(
    LevelInterface& level,
    const std::string& base_name,
    proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    return std::format(
        "{}.__auto_material_{}",
        base_name,
        static_cast<int>(render_time_enum));
}

constexpr const char* kRaytracingResolveNodeName = "RayTracingRendering";
constexpr const char* kRaytracingResolveMaterialName = "RayTraceMaterial";

bool EqualsIgnoreCaseAscii(
    const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i])))
        {
            return false;
        }
    }
    return true;
}

bool IsRaytracingRenderTime(
    LevelInterface& level,
    proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (program_id == NullId)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

bool IsExplicitRaytracingResolveNode(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    return proto_scene_mesh.render_time_enum() ==
               proto::NodeMesh::SCENE_RENDER_TIME &&
           proto_scene_mesh.has_mesh_enum() &&
           proto_scene_mesh.mesh_enum() == proto::NodeMesh::QUAD &&
           IsRaytracingRenderTime(level, proto_scene_mesh.render_time_enum()) &&
           EqualsIgnoreCaseAscii(
               proto_scene_mesh.name(),
               kRaytracingResolveNodeName);
}

EntityId CreateAutoMaterial(
    LevelInterface& level,
    const std::string& base_name,
    proto::NodeMesh::RenderTimeEnum render_time_enum,
    const std::string& explicit_name = {})
{
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (!program_id)
    {
        throw std::runtime_error(std::format(
            "No program configured for render pass {} while creating material for '{}'.",
            static_cast<int>(render_time_enum),
            base_name));
    }

    auto material = std::make_unique<frame::opengl::Material>();
    material->SetName(
        explicit_name.empty()
            ? GetAutoMaterialName(level, base_name, render_time_enum)
            : explicit_name);
    material->SetSerializeEnable(false);
    const auto material_id = level.AddMaterial(std::move(material));
    ConfigureMaterialProgramsForRenderTime(level, material_id, render_time_enum);
    return material_id;
}

bool IsRaytracingMaterial(LevelInterface& level, EntityId material_id)
{
    if (!material_id)
    {
        return false;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = material.GetProgramId(&level);
    if (!program_id)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

void ConfigureRaytracingSourceMaterial(
    LevelInterface& level, EntityId material_id)
{
    if (!IsRaytracingMaterial(level, material_id))
    {
        return;
    }
    const auto preprocess_program_id =
        level.GetRenderPassPreprocessProgramId(proto::NodeMesh::PRE_RENDER_TIME);
    if (!preprocess_program_id)
    {
        return;
    }
    level.GetMaterialFromId(material_id)
        .SetPreprocessProgramId(preprocess_program_id);
}

bool IsRaytracingSourceMaterial(LevelInterface& level, EntityId material_id)
{
    if (!IsRaytracingMaterial(level, material_id))
    {
        return false;
    }
    return level.GetMaterialFromId(material_id).GetPreprocessProgramId(&level) !=
        NullId;
}

bool IsRaytracingResolveMaterial(LevelInterface& level, EntityId material_id)
{
    if (!IsRaytracingMaterial(level, material_id))
    {
        return false;
    }
    return level.GetMaterialFromId(material_id).GetPreprocessProgramId(&level) ==
        NullId;
}

bool ShouldTreatAsRaytracingSourceNode(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    if (proto_scene_mesh.has_clean_buffer() ||
        !IsRaytracingRenderTime(level, proto_scene_mesh.render_time_enum()))
    {
        return false;
    }
    if (proto_scene_mesh.render_time_enum() == proto::NodeMesh::PRE_RENDER_TIME)
    {
        return true;
    }
    if (proto_scene_mesh.render_time_enum() !=
        proto::NodeMesh::SCENE_RENDER_TIME)
    {
        return false;
    }
    return !IsExplicitRaytracingResolveNode(level, proto_scene_mesh);
}

std::vector<std::pair<EntityId, EntityId>> GetRaytracingSourceMeshMaterials(
    LevelInterface& level)
{
    std::vector<std::pair<EntityId, EntityId>> pairs = {};
    const auto append_pairs =
        [&](proto::NodeMesh::RenderTimeEnum render_time_enum) {
            for (const auto& pair : level.GetMeshMaterialIds(render_time_enum))
            {
                if (IsRaytracingSourceMaterial(level, pair.second))
                {
                    pairs.push_back(pair);
                }
            }
        };
    append_pairs(proto::NodeMesh::PRE_RENDER_TIME);
    append_pairs(proto::NodeMesh::SCENE_RENDER_TIME);
    return pairs;
}

EntityId FindMaterialIdByName(
    LevelInterface& level, const std::string& expected_name)
{
    for (const auto material_id : level.GetMaterials())
    {
        if (level.GetNameFromId(material_id) == expected_name)
        {
            return material_id;
        }
    }
    return NullId;
}

EntityId FindRaytracingResolveMaterialId(LevelInterface& level)
{
    for (const auto& [scene_node_id, scene_material_id] :
         level.GetMeshMaterialIds(proto::NodeMesh::SCENE_RENDER_TIME))
    {
        (void)scene_node_id;
        if (IsRaytracingResolveMaterial(level, scene_material_id))
        {
            return scene_material_id;
        }
    }
    return NullId;
}

void ReplaceTextureBindingByInnerName(
    LevelInterface& level,
    MaterialInterface& material,
    const std::string& inner_name,
    EntityId texture_id)
{
    if (!texture_id)
    {
        return;
    }
    std::vector<EntityId> to_remove = {};
    for (const auto id : material.GetTextureIds())
    {
        if (material.GetInnerName(id) == inner_name)
        {
            to_remove.push_back(id);
        }
    }
    for (const auto id : to_remove)
    {
        material.RemoveTextureId(id);
    }

    EntityId bound_texture_id = texture_id;
    for (const auto existing_id : material.GetTextureIds())
    {
        if (existing_id != texture_id ||
            material.GetInnerName(existing_id) == inner_name)
        {
            continue;
        }

        auto proto_texture = level.GetTextureFromId(texture_id).ToProto();
        proto_texture.set_name(std::format(
            "{}.__binding_alias_{}_{}",
            level.GetNameFromId(texture_id),
            inner_name,
            level.GetTextures().size()));
        auto alias_texture = frame::json::ParseTexture(
            proto_texture,
            level.GetTextureFromId(texture_id).GetSize());
        alias_texture->SetName(proto_texture.name());
        alias_texture->SetSerializeEnable(false);
        bound_texture_id = level.AddTexture(std::move(alias_texture));
        break;
    }
    material.AddTextureId(bound_texture_id, inner_name);
}

EntityId FindTextureIdByInnerName(
    const MaterialInterface& material, const std::string& inner_name)
{
    for (const auto texture_id : material.GetTextureIds())
    {
        if (material.GetInnerName(texture_id) == inner_name)
        {
            return texture_id;
        }
    }
    return NullId;
}

bool IsTransmissiveRaytracingSourceMaterial(
    LevelInterface& level, EntityId material_id);

std::array<float, 4> ResolveRaytracingSourceMaterialColor(
    LevelInterface& level, EntityId material_id)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!material_id)
    {
        return kWhite;
    }
    const auto& material = level.GetMaterialFromId(material_id);
    EntityId color_texture_id = FindTextureIdByInnerName(
        material, "albedo_texture");
    if (!color_texture_id)
    {
        color_texture_id = FindTextureIdByInnerName(material, "Color");
    }
    return ReadTextureColor(level, color_texture_id);
}

std::array<float, 4> ResolveRaytracingReferenceColor(
    LevelInterface& level, bool transmissive)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)source_node_id;
        if (IsTransmissiveRaytracingSourceMaterial(level, source_material_id) ==
            transmissive)
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
        if (value < 0.0f)
        {
            value = 0.0f;
        }
        else if (value > 4.0f)
        {
            value = 4.0f;
        }
        multiplier[channel] = value;
    }
    return multiplier;
}

bool IsGeneratedGltfTextureName(const std::string& texture_name)
{
    return texture_name.find(".__gltf_tex_") != std::string::npos ||
           texture_name.find(".__gltf_solid_") != std::string::npos;
}

bool IsTransmissiveRaytracingSourceMaterial(
    LevelInterface& level, EntityId material_id)
{
    if (!material_id)
    {
        return false;
    }
    const auto& material = level.GetMaterialFromId(material_id);
    const EntityId transmission_texture_id = FindTextureIdByInnerName(
        material, "transmission_texture");
    return ReadTextureFirstChannel(level, transmission_texture_id) > 0.01f;
}

void AdoptRaytracingSceneTextures(
    LevelInterface& level,
    EntityId source_material_id,
    EntityId target_material_id,
    bool transmissive)
{
    if (!source_material_id ||
        !target_material_id ||
        source_material_id == target_material_id)
    {
        return;
    }

    auto& source = level.GetMaterialFromId(source_material_id);
    auto& target = level.GetMaterialFromId(target_material_id);

    struct Mapping
    {
        const char* source_name;
        const char* fallback_name;
        const char* target_name;
    };

    const std::array<Mapping, 7> opaque_mappings = {{
        {"albedo_texture", "Color", "opaque_albedo_texture"},
        {"normal_texture", nullptr, "opaque_normal_texture"},
        {"roughness_texture", nullptr, "opaque_roughness_texture"},
        {"metallic_texture", nullptr, "opaque_metallic_texture"},
        {"ao_texture", nullptr, "opaque_ao_texture"},
        {"specular_factor_texture", nullptr, "opaque_specular_factor_texture"},
        {"specular_color_texture", nullptr, "opaque_specular_color_texture"},
    }};
    const std::array<Mapping, 10> transmissive_mappings = {{
        {"albedo_texture", "Color", "transmissive_albedo_texture"},
        {"normal_texture", nullptr, "transmissive_normal_texture"},
        {"roughness_texture", nullptr, "transmissive_roughness_texture"},
        {"metallic_texture", nullptr, "transmissive_metallic_texture"},
        {"ao_texture", nullptr, "transmissive_ao_texture"},
        {"transmission_texture", nullptr, "transmissive_transmission_texture"},
        {"ior_texture", nullptr, "transmissive_ior_texture"},
        {"thickness_texture", nullptr, "transmissive_thickness_texture"},
        {"attenuation_color_texture", nullptr, "transmissive_attenuation_color_texture"},
        {"attenuation_distance_texture", nullptr, "transmissive_attenuation_distance_texture"},
    }};

    const auto copy_mapping =
        [&](const char* source_name,
            const char* fallback_name,
            const char* target_name) {
            EntityId source_id = FindTextureIdByInnerName(source, source_name);
            if (!source_id && fallback_name)
            {
                source_id = FindTextureIdByInnerName(source, fallback_name);
            }
            if (!source_id)
            {
                return;
            }
            const EntityId existing_target_id =
                FindTextureIdByInnerName(target, target_name);
            if (existing_target_id != NullId)
            {
                const auto existing_target_texture_name =
                    level.GetNameFromId(existing_target_id);
                const auto& existing_target_texture =
                    level.GetTextureFromId(existing_target_id);
                const bool replace_generated_target =
                    IsGeneratedGltfTextureName(existing_target_texture_name) ||
                    !existing_target_texture.SerializeEnable();
                if (!replace_generated_target)
                {
                    return;
                }
            }
            ReplaceTextureBindingByInnerName(
                level, target, target_name, source_id);
        };

    if (transmissive)
    {
        for (const auto& mapping : transmissive_mappings)
        {
            copy_mapping(
                mapping.source_name,
                mapping.fallback_name,
                mapping.target_name);
        }
        return;
    }
    for (const auto& mapping : opaque_mappings)
    {
        copy_mapping(
            mapping.source_name,
            mapping.fallback_name,
            mapping.target_name);
    }
}

template <typename T>
std::vector<T> ReadTypedBufferData(const opengl::Buffer& buffer)
{
    const auto& raw = buffer.GetRawData();
    if (raw.empty() || raw.size() % sizeof(T) != 0)
    {
        return {};
    }
    std::vector<T> result(raw.size() / sizeof(T));
    std::memcpy(result.data(), raw.data(), raw.size());
    return result;
}

std::vector<float> BuildRaytraceTriangles(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::uint32_t>& indices,
    const std::array<float, 4>& color)
{
    std::vector<float> triangles = {};
    triangles.reserve(indices.size() * 16);
    auto append_vertex = [&](const std::uint32_t index) {
        const auto point_offset = static_cast<std::size_t>(index) * 3;
        if (point_offset + 2 >= points.size())
        {
            return;
        }
        triangles.push_back(points[point_offset]);
        triangles.push_back(points[point_offset + 1]);
        triangles.push_back(points[point_offset + 2]);
        triangles.push_back(color[0]);

        if (point_offset + 2 < normals.size())
        {
            triangles.push_back(normals[point_offset]);
            triangles.push_back(normals[point_offset + 1]);
            triangles.push_back(normals[point_offset + 2]);
        }
        else
        {
            triangles.insert(triangles.end(), {0.0f, 0.0f, 0.0f});
        }
        triangles.push_back(color[1]);

        const auto texture_offset = static_cast<std::size_t>(index) * 2;
        if (texture_offset + 1 < textures.size())
        {
            triangles.push_back(textures[texture_offset]);
            triangles.push_back(textures[texture_offset + 1]);
        }
        else
        {
            triangles.insert(triangles.end(), {0.0f, 0.0f});
        }
        triangles.push_back(color[2]);
        triangles.push_back(color[3]);
    };
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        append_vertex(indices[i]);
        append_vertex(indices[i + 1]);
        append_vertex(indices[i + 2]);
    }
    return triangles;
}

std::vector<float> BuildRaytraceTriangles(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::uint32_t>& indices)
{
    return BuildRaytraceTriangles(
        points,
        normals,
        textures,
        indices,
        {1.0f, 1.0f, 1.0f, 1.0f});
}

EntityId CreateStorageBuffer(
    LevelInterface& level,
    std::size_t size,
    const void* data,
    const std::string& name)
{
    auto buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER);
    buffer->SetName(name);
    if (size == 0)
    {
        const std::uint8_t zero = 0;
        buffer->Copy(sizeof(zero), &zero);
    }
    else
    {
        buffer->Copy(size, data);
    }
    return level.AddBuffer(std::move(buffer));
}

template <typename T>
EntityId CreateStorageBuffer(
    LevelInterface& level,
    const std::vector<T>& values,
    const std::string& name)
{
    return CreateStorageBuffer(
        level,
        values.size() * sizeof(T),
        values.empty() ? nullptr : values.data(),
        name);
}

struct RaytraceAggregateBuffers
{
    EntityId triangle_buffer_id = NullId;
    EntityId bvh_buffer_id = NullId;
};

RaytraceAggregateBuffers BuildRaytraceAggregateBuffers(
    LevelInterface& level,
    bool transmissive,
    const std::string& base_name)
{
    std::vector<float> aggregate_points = {};
    std::vector<float> aggregate_normals = {};
    std::vector<float> aggregate_textures = {};
    std::vector<float> aggregate_triangles = {};
    std::vector<std::uint32_t> aggregate_indices = {};
    const auto reference_color =
        ResolveRaytracingReferenceColor(level, transmissive);

    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        if (IsTransmissiveRaytracingSourceMaterial(level, source_material_id) !=
            transmissive)
        {
            continue;
        }
        auto& node =
            dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(source_node_id));
        const auto mesh_id = node.GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        const auto& mesh = level.GetMeshFromId(mesh_id);
        if (!mesh.GetPointBufferId() || !mesh.GetIndexBufferId())
        {
            continue;
        }
        auto* point_buffer = dynamic_cast<opengl::Buffer*>(
            &level.GetBufferFromId(mesh.GetPointBufferId()));
        auto* normal_buffer = mesh.GetNormalBufferId()
            ? dynamic_cast<opengl::Buffer*>(
                  &level.GetBufferFromId(mesh.GetNormalBufferId()))
            : nullptr;
        auto* texture_buffer = mesh.GetTextureBufferId()
            ? dynamic_cast<opengl::Buffer*>(
                  &level.GetBufferFromId(mesh.GetTextureBufferId()))
            : nullptr;
        auto* index_buffer = dynamic_cast<opengl::Buffer*>(
            &level.GetBufferFromId(mesh.GetIndexBufferId()));
        if (!point_buffer || !index_buffer)
        {
            continue;
        }
        const auto points = ReadTypedBufferData<float>(*point_buffer);
        const auto normals = normal_buffer
            ? ReadTypedBufferData<float>(*normal_buffer)
            : std::vector<float>{};
        const auto textures = texture_buffer
            ? ReadTypedBufferData<float>(*texture_buffer)
            : std::vector<float>{};
        const auto indices = ReadTypedBufferData<std::uint32_t>(*index_buffer);
        if (points.empty() || indices.empty())
        {
            continue;
        }
        const auto source_color = ResolveRaytracingSourceMaterialColor(
            level, source_material_id);
        const auto color_multiplier = ResolveRaytracingColorMultiplier(
            source_color, reference_color);
        const auto base_index = static_cast<std::uint32_t>(
            aggregate_points.size() / 3);
        aggregate_points.insert(
            aggregate_points.end(),
            points.begin(),
            points.end());
        aggregate_normals.insert(
            aggregate_normals.end(),
            normals.begin(),
            normals.end());
        aggregate_textures.insert(
            aggregate_textures.end(),
            textures.begin(),
            textures.end());
        for (const auto index : indices)
        {
            aggregate_indices.push_back(base_index + index);
        }
        const auto mesh_triangles = BuildRaytraceTriangles(
            points,
            normals,
            textures,
            indices,
            color_multiplier);
        aggregate_triangles.insert(
            aggregate_triangles.end(),
            mesh_triangles.begin(),
            mesh_triangles.end());
    }

    const auto triangle_name =
        std::format("{}.triangle", base_name);
    const auto bvh_name =
        std::format("{}.bvh", base_name);
    if (aggregate_indices.empty())
    {
        return {
            CreateStorageBuffer(level, 0, nullptr, triangle_name),
            CreateStorageBuffer(level, 0, nullptr, bvh_name)};
    }

    const auto bvh_nodes = frame::BuildBVH(aggregate_points, aggregate_indices);
    return {
        CreateStorageBuffer(level, aggregate_triangles, triangle_name),
        CreateStorageBuffer(level, bvh_nodes, bvh_name)};
}

void EnsureRaytracingResolveNode(LevelInterface& level)
{
    if (!IsRaytracingRenderTime(level, proto::NodeMesh::SCENE_RENDER_TIME) ||
        FindRaytracingResolveMaterialId(level) != NullId)
    {
        return;
    }
    if (GetRaytracingSourceMeshMaterials(level).empty())
    {
        return;
    }

    auto material_id = FindMaterialIdByName(
        level, kRaytracingResolveMaterialName);
    if (material_id == NullId)
    {
        material_id = CreateAutoMaterial(
            level,
            kRaytracingResolveNodeName,
            proto::NodeMesh::SCENE_RENDER_TIME,
            kRaytracingResolveMaterialName);
    }

    const auto quad_id = level.GetDefaultMeshQuadId();
    if (quad_id == NullId)
    {
        throw std::runtime_error(
            "Default quad mesh not available for raytracing resolve node.");
    }

    auto node = std::make_unique<NodeMesh>(GetFunctor(level), quad_id);
    node->GetData().set_name(kRaytracingResolveNodeName);
    node->GetData().set_render_time_enum(proto::NodeMesh::SCENE_RENDER_TIME);
    if (const auto root_id = level.GetDefaultRootSceneNodeId(); root_id)
    {
        node->SetParentName(level.GetNameFromId(root_id));
    }
    const auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(
        scene_id,
        material_id,
        proto::NodeMesh::SCENE_RENDER_TIME);
}

void FinalizeRaytracingSceneMaterials(LevelInterface& level)
{
    EnsureRaytracingResolveNode(level);

    for (const auto& [scene_node_id, scene_material_id] :
         level.GetMeshMaterialIds(proto::NodeMesh::SCENE_RENDER_TIME))
    {
        (void)scene_node_id;
        if (!scene_material_id ||
            !IsRaytracingResolveMaterial(level, scene_material_id))
        {
            continue;
        }

        bool adopted_transmissive = false;
        bool adopted_opaque = false;
        for (const auto& [source_node_id, source_material_id] :
             GetRaytracingSourceMeshMaterials(level))
        {
            (void)source_node_id;
            const bool source_is_transmissive =
                IsTransmissiveRaytracingSourceMaterial(
                    level, source_material_id);
            if (source_is_transmissive)
            {
                if (!adopted_transmissive)
                {
                    AdoptRaytracingSceneTextures(
                        level,
                        source_material_id,
                        scene_material_id,
                        true);
                    adopted_transmissive = true;
                }
            }
            else if (!adopted_opaque)
            {
                AdoptRaytracingSceneTextures(
                    level,
                    source_material_id,
                    scene_material_id,
                    false);
                adopted_opaque = true;
            }
        }

        auto& scene_material = level.GetMaterialFromId(scene_material_id);
        const auto buffer_base_name =
            std::format("{}.scene", scene_material.GetData().name());
        const auto transmissive_buffers = BuildRaytraceAggregateBuffers(
            level,
            true,
            buffer_base_name + "_transmissive");
        const auto opaque_buffers = BuildRaytraceAggregateBuffers(
            level,
            false,
            buffer_base_name + "_opaque");
        scene_material.AddBufferName(
            level.GetNameFromId(transmissive_buffers.triangle_buffer_id),
            "TriangleBufferTransmissive");
        scene_material.AddBufferName(
            level.GetNameFromId(transmissive_buffers.bvh_buffer_id),
            "BvhBufferTransmissive");
        scene_material.AddBufferName(
            level.GetNameFromId(opaque_buffers.triangle_buffer_id),
            "TriangleBufferOpaque");
        scene_material.AddBufferName(
            level.GetNameFromId(opaque_buffers.bvh_buffer_id),
            "BvhBufferOpaque");
        const auto instance_buffer_id = CreateStorageBuffer(
            level,
            0,
            nullptr,
            buffer_base_name + "_instances");
        scene_material.AddBufferName(
            level.GetNameFromId(instance_buffer_id),
            "RaytraceInstanceBuffer");
    }
}

void ApplyAnimationPlayback(
    const proto::NodeMesh& proto_scene_mesh,
    NodeMesh& node,
    MeshInterface* mesh)
{
    node.GetData().set_play_animation(proto_scene_mesh.play_animation());
    if (proto_scene_mesh.has_animation_speed())
    {
        node.GetData().set_animation_speed(
            proto_scene_mesh.animation_speed());
    }
    if (proto_scene_mesh.has_animation_clip_name())
    {
        node.GetData().set_animation_clip_name(
            proto_scene_mesh.animation_clip_name());
    }
    if (proto_scene_mesh.has_animation_clip_index())
    {
        node.GetData().set_animation_clip_index(
            proto_scene_mesh.animation_clip_index());
    }
    if (!mesh)
    {
        return;
    }
    auto* gl_mesh = dynamic_cast<opengl::SkinnedMesh*>(mesh);
    if (!gl_mesh)
    {
        return;
    }
    const float speed = proto_scene_mesh.has_animation_speed()
                            ? proto_scene_mesh.animation_speed()
                            : 1.0f;
    gl_mesh->SetSkinningAnimation(proto_scene_mesh.play_animation(), speed);
    const std::string clip_name = proto_scene_mesh.has_animation_clip_name()
                                      ? proto_scene_mesh.animation_clip_name()
                                      : "";
    std::optional<std::uint32_t> clip_index = std::nullopt;
    if (proto_scene_mesh.has_animation_clip_index())
    {
        clip_index = proto_scene_mesh.animation_clip_index();
    }
    gl_mesh->SetSkinningAnimationClip(clip_name, clip_index);
    if (gl_mesh->HasSkinning())
    {
        if (clip_index)
        {
            Logger::GetInstance()->info(
                "Skinned mesh '{}' animation settings: play={}, speed={}, "
                "clip_name='{}', clip_index={}.",
                node.GetData().name(),
                proto_scene_mesh.play_animation(),
                speed,
                clip_name,
                *clip_index);
        }
        else
        {
            Logger::GetInstance()->info(
                "Skinned mesh '{}' animation settings: play={}, speed={}, "
                "clip_name='{}'.",
                node.GetData().name(),
                proto_scene_mesh.play_animation(),
                speed,
                clip_name);
        }
    }
}

[[nodiscard]] bool ParseNodeMatrix(
    LevelInterface& level, const proto::NodeMatrix& proto_scene_matrix)
{
    if (!proto_scene_matrix.has_matrix_type_enum())
    {
        throw std::runtime_error(std::format(
            "Node matrix '{}' must explicitly set matrix_type_enum to STATIC_MATRIX or ROTATION_MATRIX.",
            proto_scene_matrix.name()));
    }
    std::unique_ptr<frame::NodeMatrix> scene_matrix = nullptr;
    const bool rotation = proto_scene_matrix.matrix_type_enum() ==
        proto::NodeMatrix::ROTATION_MATRIX;

    if (proto_scene_matrix.has_matrix())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level),
            ParseUniform(proto_scene_matrix.matrix()),
            rotation);
    }
    else if (proto_scene_matrix.has_quaternion())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level),
            ParseUniform(proto_scene_matrix.quaternion()),
            rotation);
    }
    else
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level), glm::mat4(1.0f), rotation);
    }

    // In case the proto explicitly requested a rotation matrix but we passed
    // "rotation" as false to the constructor (shouldn't happen, but for
    // clarity), force the type here.
    if (rotation)
    {
        scene_matrix->GetData().set_matrix_type_enum(
            proto::NodeMatrix::ROTATION_MATRIX);
    }

    scene_matrix->GetData().set_name(proto_scene_matrix.name());
    scene_matrix->SetParentName(proto_scene_matrix.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_matrix));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseNodeMeshClearBuffer(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    auto node_interface = std::make_unique<frame::NodeMesh>(
        GetFunctor(level), proto_scene_mesh.clean_buffer());
    node_interface->GetData().set_name(proto_scene_mesh.name());
    auto maybe_scene_id = level.AddSceneNode(std::move(node_interface));
    if (!maybe_scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    auto scene_id = maybe_scene_id;
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    node.GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, nullptr);
    level.AddMeshMaterialId(
        scene_id, 0, proto_scene_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeMeshMeshEnum(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    if (proto_scene_mesh.mesh_enum() == proto::NodeMesh::INVALID)
    {
        throw std::runtime_error("Didn't find any mesh file name or any enum.");
    }
    // In this case there is only one material per mesh.
    EntityId mesh_id = 0;
    switch (proto_scene_mesh.mesh_enum())
    {
    case proto::NodeMesh::CUBE: {
        mesh_id = level.GetDefaultMeshCubeId();
        break;
    }
    case proto::NodeMesh::QUAD: {
        mesh_id = level.GetDefaultMeshQuadId();
        break;
    }
    default: {
        throw std::runtime_error(
            std::format(
                "unknown mesh enum value: {}",
                static_cast<int>(proto_scene_mesh.mesh_enum())));
    }
    }
    const EntityId material_id = CreateAutoMaterial(
        level,
        proto_scene_mesh.name(),
        proto_scene_mesh.render_time_enum(),
        IsExplicitRaytracingResolveNode(level, proto_scene_mesh)
            ? kRaytracingResolveMaterialName
            : "");
    if (ShouldTreatAsRaytracingSourceNode(level, proto_scene_mesh))
    {
        ConfigureRaytracingSourceMaterial(level, material_id);
    }
    auto& mesh = level.GetMeshFromId(mesh_id);
    mesh.GetData().set_render_primitive_enum(
        proto_scene_mesh.render_primitive_enum());
    std::unique_ptr<NodeMesh> node_interface =
        std::make_unique<NodeMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_mesh.name());
    node_interface->SetParentName(proto_scene_mesh.parent());
    node_interface->GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, &mesh);
    level.AddMeshMaterialId(
        scene_id, material_id, proto_scene_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeMeshFileName(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    const auto forced_program_id =
        level.GetRenderPassProgramId(proto_scene_mesh.render_time_enum());
    const auto asset_root = frame::file::FindDirectory("asset");
    auto vec_node_mesh_id = opengl::file::LoadMeshesFromFile(
        level,
        (asset_root / "model" / proto_scene_mesh.file_name()).lexically_normal(),
        proto_scene_mesh.name(),
        "",
        proto_scene_mesh.acceleration_structure_enum(),
        forced_program_id);
    if (vec_node_mesh_id.empty())
    {
        return false;
    }
    int i = 0;
    for (const auto& [node_id, gltf_material_id] : vec_node_mesh_id)
    {
        EntityId material_id = gltf_material_id;
        if (!material_id)
        {
            material_id = CreateAutoMaterial(
                level,
                std::format("{}.{}", proto_scene_mesh.name(), i),
                proto_scene_mesh.render_time_enum());
        }
        ConfigureMaterialProgramsForRenderTime(
            level,
            material_id,
            proto_scene_mesh.render_time_enum());
        if (ShouldTreatAsRaytracingSourceNode(level, proto_scene_mesh))
        {
            ConfigureRaytracingSourceMaterial(level, material_id);
        }
        auto& node = level.GetSceneNodeFromId(node_id);
        auto& mesh = level.GetMeshFromId(node.GetLocalMesh());
        mesh.GetData().set_file_name(proto_scene_mesh.file_name());
        mesh.GetData().set_render_primitive_enum(
            proto_scene_mesh.render_primitive_enum());
        auto str = std::format("{}.{}", proto_scene_mesh.name(), i);
        mesh.SetName(str);
        auto& mesh_node = dynamic_cast<NodeMesh&>(node);
        mesh_node.GetData().set_file_name(
            proto_scene_mesh.file_name());
        mesh_node.GetData().set_acceleration_structure_enum(
            proto_scene_mesh.acceleration_structure_enum());
        // Rename the node to match the reference name (without the 'Node.'
        // prefix) so serialization will use the same identifier as the input
        // file.
        if (vec_node_mesh_id.size() == 1)
        {
            mesh_node.SetName(proto_scene_mesh.name());
        }
        else
        {
            mesh_node.SetName(str);
        }
        node.SetParentName(proto_scene_mesh.parent());
        mesh_node.GetData().set_render_time_enum(
            proto_scene_mesh.render_time_enum());
        ApplyAnimationPlayback(proto_scene_mesh, mesh_node, &mesh);
        if (!material_id)
        {
            throw std::runtime_error(std::format(
                "No material mapping found for mesh {} in file {}",
                proto_scene_mesh.name(),
                proto_scene_mesh.file_name()));
        }
        level.AddMeshMaterialId(
            node_id,
            material_id,
            proto_scene_mesh.render_time_enum());
        ++i;
    }
    return true;
}

[[nodiscard]] bool ParseNodeMeshStreamInput(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    assert(proto_scene_mesh.has_multi_plugin());
    auto point_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    point_buffer->SetName("point." + proto_scene_mesh.name());
    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    normal_buffer->SetName("normal." + proto_scene_mesh.name());
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto index_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    index_buffer->SetName("index." + proto_scene_mesh.name());
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));
    auto color_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    color_buffer->SetName("color." + proto_scene_mesh.name());
    auto color_buffer_id = level.AddBuffer(std::move(color_buffer));

    // Create a new mesh.
    MeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.color_buffer_id = color_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.render_primitive_enum = proto::NodeMesh::POINT_PRIMITIVE;
    auto mesh = std::make_unique<opengl::Mesh>(level, parameter);
    mesh->SetName("mesh." + proto_scene_mesh.name());
    auto mesh_id = level.AddMesh(std::move(mesh));
    const EntityId material_id = CreateAutoMaterial(
        level,
        proto_scene_mesh.name(),
        proto_scene_mesh.render_time_enum());
    if (ShouldTreatAsRaytracingSourceNode(level, proto_scene_mesh))
    {
        ConfigureRaytracingSourceMaterial(level, material_id);
    }
    // Create the node corresponding to the mesh.
    auto& mesh_ref = level.GetMeshFromId(mesh_id);
    mesh_ref.GetData().set_render_primitive_enum(
        proto_scene_mesh.render_primitive_enum());
    std::unique_ptr<NodeMesh> node_interface =
        std::make_unique<NodeMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_mesh.name());
    node_interface->SetParentName(proto_scene_mesh.parent());
    node_interface->GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, &mesh_ref);
    level.AddMeshMaterialId(
        scene_id, material_id, proto_scene_mesh.render_time_enum());
    if (!scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    return true;
}

[[nodiscard]] bool ParseNodeMesh(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    // 1st case this is a clean mesh node.
    if (proto_scene_mesh.has_clean_buffer())
    {
        return ParseNodeMeshClearBuffer(level, proto_scene_mesh);
    }
    // 2nd case this is a enum mesh node (CUBE or QUAD).
    if (proto_scene_mesh.has_mesh_enum())
    {
        return ParseNodeMeshMeshEnum(level, proto_scene_mesh);
    }
    // 3rd case this is a mesh file.
    if (proto_scene_mesh.has_file_name())
    {
        return ParseNodeMeshFileName(level, proto_scene_mesh);
    }
    // 4th case stream input.
    if (proto_scene_mesh.has_multi_plugin())
    {
        return ParseNodeMeshStreamInput(level, proto_scene_mesh);
    }
    return false;
}

[[nodiscard]] bool ParseNodeCamera(
    LevelInterface& level, const frame::proto::NodeCamera& proto_scene_camera)
{
    if (proto_scene_camera.fov_degrees() == 0.0)
    {
        throw std::runtime_error("Need field of view degrees in camera.");
    }
    auto scene_camera = std::make_unique<frame::NodeCamera>(
        GetFunctor(level),
        ParseUniform(proto_scene_camera.position()),
        ParseUniform(proto_scene_camera.target()),
        ParseUniform(proto_scene_camera.up()),
        proto_scene_camera.fov_degrees(),
        proto_scene_camera.aspect_ratio(),
        proto_scene_camera.near_clip(),
        proto_scene_camera.far_clip());
    scene_camera->GetData().set_name(proto_scene_camera.name());
    scene_camera->SetParentName(proto_scene_camera.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_camera));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseNodeLight(
    LevelInterface& level, const proto::NodeLight& proto_scene_light)
{
    if (!proto_scene_light.has_light_type())
    {
        throw std::runtime_error(std::format(
            "Light '{}' must explicitly set light_type.",
            proto_scene_light.name()));
    }
    switch (proto_scene_light.light_type())
    {
    case proto::NodeLight::POINT_LIGHT: {
        EntityId node_id = NullId;
        auto node_light = std::make_unique<frame::NodeLight>(
            GetFunctor(level),
            LightTypeEnum::POINT_LIGHT,
            ParseUniform(proto_scene_light.position()),
            ParseUniform(proto_scene_light.color()));
        node_light->GetData().set_name(proto_scene_light.name());
        node_light->SetParentName(proto_scene_light.parent());
        node_light->GetData().set_shadow_type(proto_scene_light.shadow_type());
        node_id = level.AddSceneNode(std::move(node_light));
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::DIRECTIONAL_LIGHT: {
        EntityId node_id = NullId;
        auto node_light = std::make_unique<frame::NodeLight>(
            GetFunctor(level),
            LightTypeEnum::DIRECTIONAL_LIGHT,
            ParseUniform(proto_scene_light.direction()),
            ParseUniform(proto_scene_light.color()));
        node_light->GetData().set_name(proto_scene_light.name());
        node_light->SetParentName(proto_scene_light.parent());
        node_light->GetData().set_shadow_type(proto_scene_light.shadow_type());
        node_id = level.AddSceneNode(std::move(node_light));
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::AMBIENT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::SPOT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::INVALID_LIGHT:
        [[fallthrough]];
    default:
        throw std::runtime_error(
            std::format(
                "Unknown scene light type {}",
                static_cast<int>(proto_scene_light.light_type())));
    }
    return false;
}

} // End namespace.

[[nodiscard]] bool ParseSceneTreeFile(
    const proto::SceneTree& proto_scene_tree, LevelInterface& level)
{
    level.SetDefaultCameraName(proto_scene_tree.default_camera_name());
    level.SetDefaultRootSceneNodeName(proto_scene_tree.default_root_name());
    for (const auto& proto_matrix : proto_scene_tree.node_matrices())
    {
        if (!ParseNodeMatrix(level, proto_matrix))
        {
            return false;
        }
    }
    for (const auto& proto_mesh : proto_scene_tree.node_meshes())
    {
        if (!ParseNodeMesh(level, proto_mesh))
        {
            return false;
        }
    }
    for (const auto& proto_camera : proto_scene_tree.node_cameras())
    {
        if (!ParseNodeCamera(level, proto_camera))
        {
            return false;
        }
    }
    for (const auto& proto_light : proto_scene_tree.node_lights())
    {
        if (!ParseNodeLight(level, proto_light))
        {
            return false;
        }
    }
    FinalizeRaytracingSceneMaterials(level);
    return true;
}

} // End namespace frame::json.





