#include "frame/opengl/json/parse_scene_tree.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "frame/file/file_system.h"
#include "frame/json/parse_pixel.h"
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
        {"attenuation_color_texture", nullptr, "transmissive_attenuation_color_texture"},
        {"attenuation_distance_texture", nullptr, "transmissive_attenuation_distance_texture"},
    }};

struct RaytracingTextureAtlasLayout
{
    std::vector<EntityId> material_ids = {};
    std::unordered_map<EntityId, std::uint32_t> slot_by_material_id = {};
    std::uint32_t columns = 1u;
    std::uint32_t rows = 1u;
    std::uint32_t tile_width = 1u;
    std::uint32_t tile_height = 1u;
    std::uint32_t atlas_width = 1u;
    std::uint32_t atlas_height = 1u;
};

std::string GetRaytracingAtlasTextureName(
    LevelInterface& level,
    EntityId material_id,
    const std::string_view target_name)
{
    return std::format(
        "{}.__raytrace_atlas_{}",
        level.GetNameFromId(material_id),
        target_name);
}

bool IsRaytracingAtlasTextureName(const std::string& texture_name)
{
    return texture_name.find(".__raytrace_atlas_") != std::string::npos;
}

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

std::array<float, 4> GetDefaultRaytracingTextureColor(
    const std::string_view binding_name)
{
    if (binding_name.ends_with("normal_texture"))
    {
        return {0.5f, 0.5f, 1.0f, 1.0f};
    }
    if (binding_name.ends_with("metallic_texture") ||
        binding_name.ends_with("transmission_texture") ||
        binding_name.ends_with("thickness_texture"))
    {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    if (binding_name.ends_with("ior_texture"))
    {
        return {1.5f, 1.5f, 1.5f, 1.0f};
    }
    if (binding_name.ends_with("attenuation_distance_texture"))
    {
        return {1000000.0f, 1000000.0f, 1000000.0f, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

EntityId ResolveRaytracingMappedTextureId(
    const MaterialInterface& material,
    const RaytracingTextureMapping& mapping)
{
    EntityId texture_id = FindTextureIdByInnerName(material, mapping.source_name);
    if (!texture_id && mapping.fallback_name)
    {
        texture_id = FindTextureIdByInnerName(material, mapping.fallback_name);
    }
    return texture_id;
}

bool ShouldReplaceRaytracingSceneTextureBinding(
    LevelInterface& level,
    const MaterialInterface& material,
    const std::string_view target_name)
{
    const EntityId texture_id = FindTextureIdByInnerName(
        material,
        std::string(target_name));
    if (texture_id == NullId)
    {
        return true;
    }

    const auto texture_name = level.GetNameFromId(texture_id);
    const auto& texture = level.GetTextureFromId(texture_id);
    return IsGeneratedGltfTextureName(texture_name) ||
           IsRaytracingAtlasTextureName(texture_name) ||
           !texture.SerializeEnable() ||
           texture_name.find(".__raytrace_default_") != std::string::npos;
}

template <typename MappingArray>
std::vector<EntityId> GatherRaytracingAtlasMaterialIds(
    LevelInterface& level,
    bool transmissive,
    const MappingArray& mappings)
{
    (void)mappings;
    std::vector<EntityId> material_ids = {};
    std::unordered_set<EntityId> seen_material_ids = {};
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)source_node_id;
        if (IsTransmissiveRaytracingSourceMaterial(level, source_material_id) !=
                transmissive ||
            !source_material_id ||
            !seen_material_ids.insert(source_material_id).second)
        {
            continue;
        }
        material_ids.push_back(source_material_id);
    }
    return material_ids;
}

template <typename MappingArray>
std::optional<RaytracingTextureAtlasLayout> BuildRaytracingTextureAtlasLayout(
    LevelInterface& level,
    bool transmissive,
    const MappingArray& mappings)
{
    auto material_ids = GatherRaytracingAtlasMaterialIds(
        level,
        transmissive,
        mappings);
    if (material_ids.size() <= 1u)
    {
        return std::nullopt;
    }

    RaytracingTextureAtlasLayout layout = {};
    layout.material_ids = std::move(material_ids);
    for (std::size_t i = 0; i < layout.material_ids.size(); ++i)
    {
        layout.slot_by_material_id.emplace(
            layout.material_ids[i],
            static_cast<std::uint32_t>(i));
    }

    for (const auto material_id : layout.material_ids)
    {
        const auto& material = level.GetMaterialFromId(material_id);
        for (const auto& mapping : mappings)
        {
            const EntityId texture_id = ResolveRaytracingMappedTextureId(
                material,
                mapping);
            if (!texture_id)
            {
                continue;
            }
            const auto size = level.GetTextureFromId(texture_id).GetSize();
            layout.tile_width = std::max(layout.tile_width, std::max(1u, size.x));
            layout.tile_height = std::max(
                layout.tile_height,
                std::max(1u, size.y));
        }
    }

    const auto material_count = static_cast<double>(layout.material_ids.size());
    layout.columns = std::max(
        1u,
        static_cast<std::uint32_t>(std::ceil(std::sqrt(material_count))));
    layout.rows = std::max(
        1u,
        static_cast<std::uint32_t>(
            (layout.material_ids.size() + layout.columns - 1u) /
            layout.columns));
    layout.atlas_width = std::max(1u, layout.columns * layout.tile_width);
    layout.atlas_height = std::max(1u, layout.rows * layout.tile_height);
    return layout;
}

std::array<float, 4> GetRaytracingAtlasUvBounds(
    const RaytracingTextureAtlasLayout& layout,
    EntityId material_id)
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

std::vector<float> RemapRaytracingAtlasUvs(
    const std::vector<float>& textures,
    const std::optional<RaytracingTextureAtlasLayout>& layout,
    EntityId material_id)
{
    if (!layout || textures.empty() ||
        !layout->slot_by_material_id.contains(material_id))
    {
        return textures;
    }

    const auto bounds = GetRaytracingAtlasUvBounds(*layout, material_id);
    std::vector<float> remapped = textures;
    for (std::size_t i = 0; i + 1u < remapped.size(); i += 2u)
    {
        const float u = std::clamp(remapped[i], 0.0f, 1.0f);
        const float v = std::clamp(remapped[i + 1u], 0.0f, 1.0f);
        remapped[i] = bounds[0] + (bounds[1] - bounds[0]) * u;
        remapped[i + 1u] = bounds[2] + (bounds[3] - bounds[2]) * v;
    }
    return remapped;
}

template <typename SampleType>
std::vector<float> ConvertTextureDataToFloatRgba(
    const std::vector<SampleType>& data,
    std::size_t pixel_count,
    frame::proto::PixelStructure::Enum structure,
    float scale)
{
    std::vector<float> rgba(pixel_count * 4u, 1.0f);
    std::size_t channel_count = 4u;
    switch (structure)
    {
    case frame::proto::PixelStructure::GREY:
        channel_count = 1u;
        break;
    case frame::proto::PixelStructure::GREY_ALPHA:
        channel_count = 2u;
        break;
    case frame::proto::PixelStructure::RGB:
    case frame::proto::PixelStructure::BGR:
        channel_count = 3u;
        break;
    case frame::proto::PixelStructure::RGB_ALPHA:
    case frame::proto::PixelStructure::BGR_ALPHA:
    default:
        channel_count = 4u;
        break;
    }
    if (data.size() < pixel_count * channel_count)
    {
        return rgba;
    }

    for (std::size_t pixel = 0; pixel < pixel_count; ++pixel)
    {
        const std::size_t src = pixel * channel_count;
        const auto sample = [&](std::size_t index) {
            return static_cast<float>(data[src + index]) / scale;
        };

        float red = 1.0f;
        float green = 1.0f;
        float blue = 1.0f;
        float alpha = 1.0f;
        switch (structure)
        {
        case frame::proto::PixelStructure::GREY:
            red = green = blue = sample(0u);
            break;
        case frame::proto::PixelStructure::GREY_ALPHA:
            red = green = blue = sample(0u);
            alpha = sample(1u);
            break;
        case frame::proto::PixelStructure::RGB:
            red = sample(0u);
            green = sample(1u);
            blue = sample(2u);
            break;
        case frame::proto::PixelStructure::RGB_ALPHA:
            red = sample(0u);
            green = sample(1u);
            blue = sample(2u);
            alpha = sample(3u);
            break;
        case frame::proto::PixelStructure::BGR:
            red = sample(2u);
            green = sample(1u);
            blue = sample(0u);
            break;
        case frame::proto::PixelStructure::BGR_ALPHA:
            red = sample(2u);
            green = sample(1u);
            blue = sample(0u);
            alpha = sample(3u);
            break;
        default:
            break;
        }

        const std::size_t dst = pixel * 4u;
        rgba[dst + 0u] = red;
        rgba[dst + 1u] = green;
        rgba[dst + 2u] = blue;
        rgba[dst + 3u] = alpha;
    }
    return rgba;
}

std::vector<float> ConvertTextureToFloatRgba(TextureInterface& texture)
{
    const auto size = texture.GetSize();
    const std::size_t pixel_count =
        static_cast<std::size_t>(size.x) * size.y;
    const auto structure = texture.GetData().pixel_structure().value();
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT:
        return ConvertTextureDataToFloatRgba(
            texture.GetTextureFloat(),
            pixel_count,
            structure,
            1.0f);
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF:
        return ConvertTextureDataToFloatRgba(
            texture.GetTextureWord(),
            pixel_count,
            structure,
            65535.0f);
    case frame::proto::PixelElementSize::BYTE:
    default:
        return ConvertTextureDataToFloatRgba(
            texture.GetTextureByte(),
            pixel_count,
            structure,
            255.0f);
    }
}

void FillRaytracingAtlasTile(
    std::vector<float>& atlas_pixels,
    const RaytracingTextureAtlasLayout& layout,
    std::uint32_t slot,
    const std::array<float, 4>& color)
{
    const std::uint32_t column = slot % layout.columns;
    const std::uint32_t row = slot / layout.columns;
    const std::uint32_t start_x = column * layout.tile_width;
    const std::uint32_t start_y = row * layout.tile_height;
    for (std::uint32_t y = 0; y < layout.tile_height; ++y)
    {
        for (std::uint32_t x = 0; x < layout.tile_width; ++x)
        {
            const std::size_t dst =
                (static_cast<std::size_t>(start_y + y) * layout.atlas_width +
                 (start_x + x)) *
                4u;
            atlas_pixels[dst + 0u] = color[0];
            atlas_pixels[dst + 1u] = color[1];
            atlas_pixels[dst + 2u] = color[2];
            atlas_pixels[dst + 3u] = color[3];
        }
    }
}

void CopyTextureIntoRaytracingAtlas(
    std::vector<float>& atlas_pixels,
    const RaytracingTextureAtlasLayout& layout,
    std::uint32_t slot,
    TextureInterface& texture)
{
    const auto size = texture.GetSize();
    if (size.x == 0u || size.y == 0u)
    {
        return;
    }

    const auto rgba = ConvertTextureToFloatRgba(texture);
    const std::uint32_t column = slot % layout.columns;
    const std::uint32_t row = slot / layout.columns;
    const std::uint32_t start_x = column * layout.tile_width;
    const std::uint32_t start_y = row * layout.tile_height;
    for (std::uint32_t y = 0; y < layout.tile_height; ++y)
    {
        const std::uint32_t source_y = std::min(
            size.y - 1u,
            static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(y) * size.y) /
                layout.tile_height));
        for (std::uint32_t x = 0; x < layout.tile_width; ++x)
        {
            const std::uint32_t source_x = std::min(
                size.x - 1u,
                static_cast<std::uint32_t>(
                    (static_cast<std::uint64_t>(x) * size.x) /
                    layout.tile_width));
            const std::size_t src =
                (static_cast<std::size_t>(source_y) * size.x + source_x) * 4u;
            const std::size_t dst =
                (static_cast<std::size_t>(start_y + y) * layout.atlas_width +
                 (start_x + x)) *
                4u;
            atlas_pixels[dst + 0u] = rgba[src + 0u];
            atlas_pixels[dst + 1u] = rgba[src + 1u];
            atlas_pixels[dst + 2u] = rgba[src + 2u];
            atlas_pixels[dst + 3u] = rgba[src + 3u];
        }
    }
}

EntityId CreateRaytracingAtlasTexture(
    LevelInterface& level,
    const std::string& texture_name,
    const RaytracingTextureAtlasLayout& layout,
    const std::vector<float>& atlas_pixels)
{
    frame::proto::Texture proto_texture;
    proto_texture.set_name(texture_name);
    proto_texture.mutable_pixel_element_size()->CopyFrom(
        frame::json::PixelElementSize_FLOAT());
    proto_texture.mutable_pixel_structure()->CopyFrom(
        frame::json::PixelStructure_RGB_ALPHA());
    proto_texture.mutable_size()->set_x(static_cast<int>(layout.atlas_width));
    proto_texture.mutable_size()->set_y(static_cast<int>(layout.atlas_height));
    proto_texture.set_pixels(
        reinterpret_cast<const char*>(atlas_pixels.data()),
        static_cast<int>(atlas_pixels.size() * sizeof(float)));

    auto atlas_texture = frame::json::ParseTexture(
        proto_texture,
        {layout.atlas_width, layout.atlas_height});
    atlas_texture->SetName(texture_name);
    atlas_texture->SetSerializeEnable(false);
    return level.AddTexture(std::move(atlas_texture));
}

template <typename MappingArray>
bool BuildAndBindRaytracingTextureAtlases(
    LevelInterface& level,
    EntityId scene_material_id,
    const MappingArray& mappings,
    bool transmissive)
{
    if (!scene_material_id)
    {
        return false;
    }

    auto layout = BuildRaytracingTextureAtlasLayout(
        level,
        transmissive,
        mappings);
    if (!layout)
    {
        return false;
    }

    auto& scene_material = level.GetMaterialFromId(scene_material_id);
    for (const auto& mapping : mappings)
    {
        if (!ShouldReplaceRaytracingSceneTextureBinding(
                level,
                scene_material,
                mapping.target_name))
        {
            return false;
        }
    }

    for (const auto& mapping : mappings)
    {
        std::vector<float> atlas_pixels(
            static_cast<std::size_t>(layout->atlas_width) *
                layout->atlas_height * 4u,
            0.0f);
        const auto fallback_color = GetDefaultRaytracingTextureColor(
            mapping.target_name);
        for (const auto material_id : layout->material_ids)
        {
            const std::uint32_t slot = layout->slot_by_material_id.at(material_id);
            const auto& source_material = level.GetMaterialFromId(material_id);
            const EntityId texture_id = ResolveRaytracingMappedTextureId(
                source_material,
                mapping);
            if (!texture_id)
            {
                FillRaytracingAtlasTile(
                    atlas_pixels,
                    *layout,
                    slot,
                    fallback_color);
                continue;
            }

            auto& texture = level.GetTextureFromId(texture_id);
            CopyTextureIntoRaytracingAtlas(
                atlas_pixels,
                *layout,
                slot,
                texture);
        }

        const auto atlas_texture_id = CreateRaytracingAtlasTexture(
            level,
            GetRaytracingAtlasTextureName(
                level,
                scene_material_id,
                mapping.target_name),
            *layout,
            atlas_pixels);
        ReplaceTextureBindingByInnerName(
            level,
            scene_material,
            mapping.target_name,
            atlas_texture_id);
    }
    return true;
}

bool HasBoundRaytracingTextureAtlas(
    LevelInterface& level,
    EntityId scene_material_id,
    bool transmissive)
{
    if (!scene_material_id)
    {
        return false;
    }

    const auto& scene_material = level.GetMaterialFromId(scene_material_id);
    const auto target_name = transmissive
        ? "transmissive_albedo_texture"
        : "opaque_albedo_texture";
    const EntityId texture_id = FindTextureIdByInnerName(
        scene_material,
        target_name);
    if (!texture_id)
    {
        return false;
    }
    return IsRaytracingAtlasTextureName(level.GetNameFromId(texture_id));
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
            if (!ShouldReplaceRaytracingSceneTextureBinding(
                    level,
                    target,
                    target_name))
            {
                return;
            }
            ReplaceTextureBindingByInnerName(
                level, target, target_name, source_id);
        };

    if (transmissive)
    {
        for (const auto& mapping : kTransmissiveRaytracingTextureMappings)
        {
            copy_mapping(
                mapping.source_name,
                mapping.fallback_name,
                mapping.target_name);
        }
        return;
    }
    for (const auto& mapping : kOpaqueRaytracingTextureMappings)
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
    const std::string& name,
    opengl::BufferUsageEnum buffer_usage = opengl::BufferUsageEnum::STATIC_DRAW)
{
    auto buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER,
        buffer_usage);
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
    const std::string& name,
    opengl::BufferUsageEnum buffer_usage = opengl::BufferUsageEnum::STATIC_DRAW)
{
    return CreateStorageBuffer(
        level,
        values.size() * sizeof(T),
        values.empty() ? nullptr : values.data(),
        name,
        buffer_usage);
}

struct RaytraceAggregateBuffers
{
    EntityId triangle_buffer_id = NullId;
    EntityId bvh_buffer_id = NullId;
};

RaytraceAggregateBuffers BuildRaytraceAggregateBuffers(
    LevelInterface& level,
    EntityId scene_material_id,
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
    const auto atlas_layout = HasBoundRaytracingTextureAtlas(
        level,
        scene_material_id,
        transmissive)
        ? (transmissive
               ? BuildRaytracingTextureAtlasLayout(
                     level,
                     true,
                     kTransmissiveRaytracingTextureMappings)
               : BuildRaytracingTextureAtlasLayout(
                     level,
                     false,
                     kOpaqueRaytracingTextureMappings))
        : std::nullopt;

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
        const auto remapped_textures = RemapRaytracingAtlasUvs(
            textures,
            atlas_layout,
            source_material_id);
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
            remapped_textures.begin(),
            remapped_textures.end());
        for (const auto index : indices)
        {
            aggregate_indices.push_back(base_index + index);
        }
        const auto mesh_triangles = BuildRaytraceTriangles(
            points,
            normals,
            remapped_textures,
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
            CreateStorageBuffer(
                level,
                0,
                nullptr,
                triangle_name,
                opengl::BufferUsageEnum::DYNAMIC_DRAW),
            CreateStorageBuffer(
                level,
                0,
                nullptr,
                bvh_name,
                opengl::BufferUsageEnum::DYNAMIC_DRAW)};
    }

    const auto bvh_nodes = frame::BuildBVH(aggregate_points, aggregate_indices);
    return {
        CreateStorageBuffer(
            level,
            aggregate_triangles,
            triangle_name,
            opengl::BufferUsageEnum::DYNAMIC_DRAW),
        CreateStorageBuffer(
            level,
            bvh_nodes,
            bvh_name,
            opengl::BufferUsageEnum::DYNAMIC_DRAW)};
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

        bool adopted_transmissive = BuildAndBindRaytracingTextureAtlases(
            level,
            scene_material_id,
            kTransmissiveRaytracingTextureMappings,
            true);
        bool adopted_opaque = BuildAndBindRaytracingTextureAtlases(
            level,
            scene_material_id,
            kOpaqueRaytracingTextureMappings,
            false);
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
            scene_material_id,
            true,
            buffer_base_name + "_transmissive");
        const auto opaque_buffers = BuildRaytraceAggregateBuffers(
            level,
            scene_material_id,
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
            buffer_base_name + "_instances",
            opengl::BufferUsageEnum::DYNAMIC_DRAW);
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





