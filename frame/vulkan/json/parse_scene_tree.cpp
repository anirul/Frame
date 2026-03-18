#include "frame/vulkan/json/parse_scene_tree.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "frame/json/parse_pixel.h"
#include "frame/json/program_key.h"
#include "frame/json/parse_uniform.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/bvh.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/material.h"
#include "frame/vulkan/skinned_mesh.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan::json
{

namespace
{

std::function<NodeInterface*(const std::string&)> MakeResolver(
    LevelInterface& level)
{
    return [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
        {
            throw std::runtime_error(std::format(
                "Unable to resolve scene node named {}", name));
        }
        return &level.GetSceneNodeFromId(maybe_id);
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

bool EndsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
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

float ReadTextureFirstChannel(
    LevelInterface& level, EntityId texture_id)
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

struct GeneratedTextureSpec
{
    glm::vec4 color = glm::vec4(0.0f);
    frame::proto::PixelElementSize element_size =
        frame::json::PixelElementSize_BYTE();
};

glm::uvec2 ResolveTextureDisplaySize(LevelInterface& level);

bool IsRaytracingProgram(const ProgramInterface& program)
{
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

std::optional<GeneratedTextureSpec> GetDefaultRaytracingTextureSpec(
    const std::string& binding_name)
{
    if (EndsWith(binding_name, "albedo_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "normal_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(0.5f, 0.5f, 1.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "roughness_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "metallic_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "ao_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "specular_factor_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "specular_color_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "transmission_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "ior_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.5f, 1.5f, 1.5f, 1.0f),
            .element_size = frame::json::PixelElementSize_FLOAT()};
    }
    if (EndsWith(binding_name, "thickness_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_FLOAT()};
    }
    if (EndsWith(binding_name, "attenuation_color_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_BYTE()};
    }
    if (EndsWith(binding_name, "attenuation_distance_texture"))
    {
        return GeneratedTextureSpec{
            .color = glm::vec4(1000000.0f, 1000000.0f, 1000000.0f, 1.0f),
            .element_size = frame::json::PixelElementSize_FLOAT()};
    }
    return std::nullopt;
}

EntityId EnsureDefaultRaytracingTexture(
    LevelInterface& level, const std::string& binding_name)
{
    const auto spec = GetDefaultRaytracingTextureSpec(binding_name);
    if (!spec)
    {
        return NullId;
    }

    const std::string texture_name =
        std::format(".__raytrace_default_{}", binding_name);
    if (const EntityId existing = FindTextureIdByName(level, texture_name))
    {
        return existing;
    }

    frame::proto::Texture proto_texture;
    proto_texture.set_name(texture_name);
    proto_texture.mutable_pixel_element_size()->CopyFrom(spec->element_size);
    proto_texture.mutable_pixel_structure()->CopyFrom(
        frame::json::PixelStructure_RGB_ALPHA());
    proto_texture.mutable_size()->set_x(1);
    proto_texture.mutable_size()->set_y(1);
    if (spec->element_size.value() ==
        frame::proto::PixelElementSize::FLOAT)
    {
        std::array<float, 4> pixels = {
            spec->color.x,
            spec->color.y,
            spec->color.z,
            spec->color.w};
        proto_texture.set_pixels(
            reinterpret_cast<const char*>(pixels.data()),
            static_cast<int>(pixels.size() * sizeof(float)));
    }
    else
    {
        std::array<std::uint8_t, 4> pixels = {
            static_cast<std::uint8_t>(
                std::clamp(spec->color.x, 0.0f, 1.0f) * 255.0f),
            static_cast<std::uint8_t>(
                std::clamp(spec->color.y, 0.0f, 1.0f) * 255.0f),
            static_cast<std::uint8_t>(
                std::clamp(spec->color.z, 0.0f, 1.0f) * 255.0f),
            static_cast<std::uint8_t>(
                std::clamp(spec->color.w, 0.0f, 1.0f) * 255.0f)};
        proto_texture.set_pixels(
            reinterpret_cast<const char*>(pixels.data()),
            static_cast<int>(pixels.size()));
    }

    auto texture = frame::vulkan::json::ParseTexture(
        proto_texture, ResolveTextureDisplaySize(level));
    texture->SetName(texture_name);
    texture->SetSerializeEnable(false);
    return level.AddTexture(std::move(texture));
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
        EntityId texture_id = FindTextureIdByName(level, binding.name());
        if (!texture_id && IsRaytracingProgram(program))
        {
            texture_id = EnsureDefaultRaytracingTexture(level, binding.name());
        }
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
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum)
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
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (program_id != NullId)
    {
        const auto& program = level.GetProgramFromId(program_id);
        const auto key = frame::json::ResolveProgramKey(program.GetData());
        if (render_time_enum == frame::proto::NodeMesh::SCENE_RENDER_TIME &&
            frame::json::IsRaytracingProgramKey(key))
        {
            return "RayTraceMaterial";
        }
    }
    return std::format(
        "{}.__auto_material_{}",
        base_name,
        static_cast<int>(render_time_enum));
}

EntityId CreateAutoMaterial(
    LevelInterface& level,
    const std::string& base_name,
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (!program_id)
    {
        throw std::runtime_error(std::format(
            "No program configured for render pass {} while creating material for '{}'.",
            static_cast<int>(render_time_enum),
            base_name));
    }

    auto material = std::make_unique<frame::vulkan::Material>();
    material->SetName(GetAutoMaterialName(level, base_name, render_time_enum));
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
        auto alias_texture = frame::vulkan::json::ParseTexture(
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
                    !existing_target_texture.SerializeEnable() ||
                    existing_target_texture_name.find(".__raytrace_default_") !=
                        std::string::npos;
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
std::vector<T> ReadTypedBufferData(const frame::vulkan::Buffer& buffer)
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
    const std::vector<std::uint32_t>& indices);

EntityId CreateStorageBuffer(
    LevelInterface& level,
    std::size_t size,
    const void* data,
    const std::string& name)
{
    auto buffer = std::make_unique<frame::vulkan::Buffer>();
    buffer->SetName(name);
    buffer->Copy(size, data);
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
    const std::string& base_name,
    bool build_software_bvh)
{
    std::vector<float> aggregate_points = {};
    std::vector<float> aggregate_normals = {};
    std::vector<float> aggregate_textures = {};
    std::vector<std::uint32_t> aggregate_indices = {};

    for (const auto& [pre_node_id, pre_material_id] :
         level.GetMeshMaterialIds(frame::proto::NodeMesh::PRE_RENDER_TIME))
    {
        if (IsTransmissiveRaytracingSourceMaterial(level, pre_material_id) !=
            transmissive)
        {
            continue;
        }
        auto& node = dynamic_cast<frame::NodeMesh&>(
            level.GetSceneNodeFromId(pre_node_id));
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
        auto* point_buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level.GetBufferFromId(mesh.GetPointBufferId()));
        auto* normal_buffer = mesh.GetNormalBufferId()
            ? dynamic_cast<frame::vulkan::Buffer*>(
                  &level.GetBufferFromId(mesh.GetNormalBufferId()))
            : nullptr;
        auto* texture_buffer = mesh.GetTextureBufferId()
            ? dynamic_cast<frame::vulkan::Buffer*>(
                  &level.GetBufferFromId(mesh.GetTextureBufferId()))
            : nullptr;
        auto* index_buffer = dynamic_cast<frame::vulkan::Buffer*>(
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
    }

    const auto triangle_name = std::format("{}.triangle", base_name);
    const auto bvh_name = std::format("{}.bvh", base_name);
    if (aggregate_indices.empty())
    {
        return {
            CreateStorageBuffer(level, 0, nullptr, triangle_name),
            CreateStorageBuffer(level, 0, nullptr, bvh_name)};
    }

    const auto triangles = BuildRaytraceTriangles(
        aggregate_points,
        aggregate_normals,
        aggregate_textures,
        aggregate_indices);
    if (!build_software_bvh)
    {
        return {
            CreateStorageBuffer(level, triangles, triangle_name),
            CreateStorageBuffer(level, 0, nullptr, bvh_name)};
    }
    const auto bvh_nodes = frame::BuildBVH(aggregate_points, aggregate_indices);
    return {
        CreateStorageBuffer(level, triangles, triangle_name),
        CreateStorageBuffer(level, bvh_nodes, bvh_name)};
}

void FinalizeRaytracingSceneMaterials(
    LevelInterface& level,
    bool prefer_hardware_raytracing)
{
    const bool build_software_bvh = !prefer_hardware_raytracing;
    for (const auto& [scene_node_id, scene_material_id] :
         level.GetMeshMaterialIds(frame::proto::NodeMesh::SCENE_RENDER_TIME))
    {
        (void)scene_node_id;
        if (!scene_material_id ||
            !IsRaytracingMaterial(level, scene_material_id))
        {
            continue;
        }

        bool adopted_transmissive = false;
        bool adopted_opaque = false;
        for (const auto& [pre_node_id, pre_material_id] :
             level.GetMeshMaterialIds(frame::proto::NodeMesh::PRE_RENDER_TIME))
        {
            (void)pre_node_id;
            const bool source_is_transmissive =
                IsTransmissiveRaytracingSourceMaterial(level, pre_material_id);
            if (source_is_transmissive)
            {
                if (!adopted_transmissive)
                {
                    AdoptRaytracingSceneTextures(
                        level,
                        pre_material_id,
                        scene_material_id,
                        true);
                    adopted_transmissive = true;
                }
            }
            else if (!adopted_opaque)
            {
                AdoptRaytracingSceneTextures(
                    level,
                    pre_material_id,
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
            buffer_base_name + "_transmissive",
            build_software_bvh);
        const auto opaque_buffers = BuildRaytraceAggregateBuffers(
            level,
            false,
            buffer_base_name + "_opaque",
            build_software_bvh);
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
    }
}

void GatherNodeMeshTransforms(
    const aiNode* node,
    const aiMatrix4x4& parent_transform,
    std::unordered_map<unsigned int, aiMatrix4x4>& mesh_transforms)
{
    if (!node)
    {
        return;
    }
    const aiMatrix4x4 global_transform = parent_transform * node->mTransformation;
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        const unsigned int mesh_index = node->mMeshes[i];
        if (!mesh_transforms.contains(mesh_index))
        {
            mesh_transforms.emplace(mesh_index, global_transform);
        }
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        GatherNodeMeshTransforms(
            node->mChildren[i],
            global_transform,
            mesh_transforms);
    }
}

EntityId SelectGltfProgramId(LevelInterface& level)
{
    EntityId first_program_id = NullId;
    EntityId first_scene_program_id = NullId;
    EntityId first_quad_program_id = NullId;
    EntityId raytrace_scene_program_id = NullId;
    EntityId raytrace_quad_program_id = NullId;
    for (const auto program_id : level.GetPrograms())
    {
        if (!first_program_id)
        {
            first_program_id = program_id;
        }
        const auto& program = level.GetProgramFromId(program_id);
        const auto scene_type = program.GetData().input_scene_type().value();
        if (scene_type == frame::proto::SceneType::SCENE &&
            !first_scene_program_id)
        {
            first_scene_program_id = program_id;
        }
        if (scene_type == frame::proto::SceneType::QUAD &&
            !first_quad_program_id)
        {
            first_quad_program_id = program_id;
        }
        const auto key = frame::json::ResolveProgramKey(program.GetData());
        if (!frame::json::IsRaytracingProgramKey(key))
        {
            continue;
        }
        if (scene_type == frame::proto::SceneType::SCENE &&
            !raytrace_scene_program_id)
        {
            raytrace_scene_program_id = program_id;
        }
        if (scene_type == frame::proto::SceneType::QUAD &&
            !raytrace_quad_program_id)
        {
            raytrace_quad_program_id = program_id;
        }
    }
    if (raytrace_quad_program_id)
    {
        return raytrace_quad_program_id;
    }
    if (raytrace_scene_program_id)
    {
        return raytrace_scene_program_id;
    }
    if (first_scene_program_id)
    {
        return first_scene_program_id;
    }
    if (first_quad_program_id)
    {
        return first_quad_program_id;
    }
    return first_program_id;
}

glm::uvec2 ResolveTextureDisplaySize(LevelInterface& level)
{
    const auto output_id = level.GetDefaultOutputTextureId();
    if (!output_id)
    {
        return {1u, 1u};
    }
    return level.GetTextureFromId(output_id).GetSize();
}

struct MaterialTextureSource
{
    std::optional<std::filesystem::path> file_path = std::nullopt;
    const aiTexture* embedded_texture = nullptr;
    std::string cache_key = {};
};

std::optional<MaterialTextureSource> ResolveMaterialTextureSource(
    const aiScene* scene,
    const std::filesystem::path& model_path,
    const aiString& texture_path)
{
    const std::string raw = texture_path.C_Str();
    if (raw.empty())
    {
        return std::nullopt;
    }
    if (raw.front() == '*')
    {
        if (!scene)
        {
            return std::nullopt;
        }
        const aiTexture* embedded_texture =
            scene->GetEmbeddedTexture(raw.c_str());
        if (!embedded_texture)
        {
            return std::nullopt;
        }
        return MaterialTextureSource{
            .embedded_texture = embedded_texture,
            .cache_key = raw};
    }
    const std::filesystem::path parsed(raw);
    if (parsed.is_absolute() && std::filesystem::exists(parsed))
    {
        return MaterialTextureSource{
            .file_path = parsed.lexically_normal(),
            .cache_key = parsed.lexically_normal().generic_string()};
    }
    const auto local = (model_path.parent_path() / parsed).lexically_normal();
    if (std::filesystem::exists(local))
    {
        return MaterialTextureSource{
            .file_path = local,
            .cache_key = local.generic_string()};
    }
    try
    {
        const auto resolved = frame::file::FindFile(parsed).lexically_normal();
        return MaterialTextureSource{
            .file_path = resolved,
            .cache_key = resolved.generic_string()};
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<MaterialTextureSource> FindFirstMaterialTextureSource(
    const aiScene* scene,
    const aiMaterial* material,
    const std::filesystem::path& model_path,
    std::initializer_list<aiTextureType> types)
{
    if (!material)
    {
        return std::nullopt;
    }
    for (const auto type : types)
    {
        if (material->GetTextureCount(type) == 0)
        {
            continue;
        }
        aiString texture_path;
        if (material->GetTexture(type, 0, &texture_path) != aiReturn_SUCCESS)
        {
            continue;
        }
        auto resolved = ResolveMaterialTextureSource(
            scene, model_path, texture_path);
        if (resolved)
        {
            return resolved;
        }
    }
    return std::nullopt;
}

std::optional<MaterialTextureSource> FindMaterialTextureSource(
    const aiScene* scene,
    const aiMaterial* material,
    const std::filesystem::path& model_path,
    const aiTextureType type,
    const unsigned int index)
{
    if (!material || material->GetTextureCount(type) <= index)
    {
        return std::nullopt;
    }
    aiString texture_path;
    if (material->GetTexture(type, index, &texture_path) != aiReturn_SUCCESS)
    {
        return std::nullopt;
    }
    return ResolveMaterialTextureSource(scene, model_path, texture_path);
}

glm::vec4 ReadBaseColorFactor(const aiMaterial* material)
{
    aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
    if (material &&
        material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS)
    {
        return {color.r, color.g, color.b, color.a};
    }
    aiColor3D diffuse(1.0f, 1.0f, 1.0f);
    if (material &&
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == aiReturn_SUCCESS)
    {
        return {diffuse.r, diffuse.g, diffuse.b, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

float ReadRoughnessFactor(const aiMaterial* material)
{
    ai_real roughness = 1.0f;
    if (material &&
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) ==
            aiReturn_SUCCESS)
    {
        return std::clamp(static_cast<float>(roughness), 0.0f, 1.0f);
    }
    return 1.0f;
}

float ReadMetallicFactor(const aiMaterial* material)
{
    ai_real metallic = 0.0f;
    if (material &&
        material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) ==
            aiReturn_SUCCESS)
    {
        return std::clamp(static_cast<float>(metallic), 0.0f, 1.0f);
    }
    return 0.0f;
}

float ReadTransmissionFactor(const aiMaterial* material)
{
    ai_real transmission = 0.0f;
    if (material &&
        material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission) ==
            aiReturn_SUCCESS)
    {
        return std::clamp(static_cast<float>(transmission), 0.0f, 1.0f);
    }
    return 0.0f;
}

float ReadSpecularFactor(const aiMaterial* material)
{
    ai_real specular = 1.0f;
    if (material &&
        material->Get(AI_MATKEY_SPECULAR_FACTOR, specular) ==
            aiReturn_SUCCESS)
    {
        return std::clamp(static_cast<float>(specular), 0.0f, 1.0f);
    }
    return 1.0f;
}

glm::vec3 ReadSpecularColor(const aiMaterial* material)
{
    aiColor3D color(1.0f, 1.0f, 1.0f);
    if (material &&
        material->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn_SUCCESS)
    {
        return {
            std::clamp(color.r, 0.0f, 1.0f),
            std::clamp(color.g, 0.0f, 1.0f),
            std::clamp(color.b, 0.0f, 1.0f)};
    }
    return {1.0f, 1.0f, 1.0f};
}

float ReadIorFactor(const aiMaterial* material)
{
    ai_real ior = 1.5f;
    if (material && material->Get(AI_MATKEY_REFRACTI, ior) == aiReturn_SUCCESS)
    {
        return std::max(static_cast<float>(ior), 1.0f);
    }
    return 1.5f;
}

float ReadThicknessFactor(const aiMaterial* material)
{
    ai_real thickness = 0.0f;
    if (material &&
        material->Get(AI_MATKEY_VOLUME_THICKNESS_FACTOR, thickness) ==
            aiReturn_SUCCESS)
    {
        return std::max(static_cast<float>(thickness), 0.0f);
    }
    return 0.0f;
}

float ReadAttenuationDistance(const aiMaterial* material)
{
    ai_real distance = 1000000.0f;
    if (material &&
        material->Get(AI_MATKEY_VOLUME_ATTENUATION_DISTANCE, distance) ==
            aiReturn_SUCCESS)
    {
        return std::max(static_cast<float>(distance), 0.0001f);
    }
    return 1000000.0f;
}

glm::vec3 ReadAttenuationColor(const aiMaterial* material)
{
    aiColor3D color(1.0f, 1.0f, 1.0f);
    if (material &&
        material->Get(AI_MATKEY_VOLUME_ATTENUATION_COLOR, color) ==
            aiReturn_SUCCESS)
    {
        return {
            std::clamp(color.r, 0.0f, 1.0f),
            std::clamp(color.g, 0.0f, 1.0f),
            std::clamp(color.b, 0.0f, 1.0f)};
    }
    return {1.0f, 1.0f, 1.0f};
}

glm::mat4 AiToGlm(const aiMatrix4x4& matrix)
{
    return glm::mat4(
        matrix.a1,
        matrix.b1,
        matrix.c1,
        matrix.d1,
        matrix.a2,
        matrix.b2,
        matrix.c2,
        matrix.d2,
        matrix.a3,
        matrix.b3,
        matrix.c3,
        matrix.d3,
        matrix.a4,
        matrix.b4,
        matrix.c4,
        matrix.d4);
}

struct NodeAnimationChannel
{
    std::vector<aiVectorKey> position_keys = {};
    std::vector<aiQuatKey> rotation_keys = {};
    std::vector<aiVectorKey> scaling_keys = {};
};

struct AnimationClipData
{
    std::string name = {};
    std::unordered_map<int, NodeAnimationChannel> channels = {};
    double duration_ticks = 0.0;
    double ticks_per_second = 25.0;
    bool has_animation = false;
};

struct SkinAnimationData
{
    struct NodeData
    {
        std::string name = {};
        int parent = -1;
        aiMatrix4x4 bind_local_transform = aiMatrix4x4();
        std::vector<int> children = {};
    };

    struct BoneData
    {
        int node_index = -1;
        aiMatrix4x4 offset_matrix = aiMatrix4x4();
    };

    std::vector<NodeData> nodes = {};
    std::unordered_map<std::string, int> node_indices = {};
    std::vector<BoneData> bones = {};
    std::vector<AnimationClipData> clips = {};
    std::unordered_map<std::string, std::size_t> clip_name_to_index = {};
    aiMatrix4x4 global_inverse_transform = aiMatrix4x4();
};

void BuildNodeHierarchy(
    const aiNode* node,
    int parent,
    std::vector<SkinAnimationData::NodeData>& nodes,
    std::unordered_map<std::string, int>& node_indices)
{
    if (!node)
    {
        return;
    }

    const int node_index = static_cast<int>(nodes.size());
    SkinAnimationData::NodeData node_data;
    node_data.name = node->mName.C_Str();
    node_data.parent = parent;
    node_data.bind_local_transform = node->mTransformation;
    nodes.push_back(std::move(node_data));
    node_indices[nodes.back().name] = node_index;

    if (parent >= 0 && parent < static_cast<int>(nodes.size()))
    {
        nodes[parent].children.push_back(node_index);
    }

    for (unsigned int child = 0; child < node->mNumChildren; ++child)
    {
        BuildNodeHierarchy(node->mChildren[child], node_index, nodes, node_indices);
    }
}

const AnimationClipData* SelectAnimationClip(
    const SkinAnimationData& animation_data,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index)
{
    if (animation_data.clips.empty())
    {
        return nullptr;
    }
    if (!clip_name.empty())
    {
        const auto key = ToLowerAscii(clip_name);
        if (auto it = animation_data.clip_name_to_index.find(key);
            it != animation_data.clip_name_to_index.end() &&
            it->second < animation_data.clips.size())
        {
            return &animation_data.clips[it->second];
        }
    }
    if (clip_index && *clip_index < animation_data.clips.size())
    {
        return &animation_data.clips[*clip_index];
    }
    return &animation_data.clips.front();
}

std::size_t FindVectorKeyIndex(
    const std::vector<aiVectorKey>& keys,
    double time_ticks)
{
    if (keys.size() < 2)
    {
        return 0;
    }
    for (std::size_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (time_ticks < keys[i + 1].mTime)
        {
            return i;
        }
    }
    return keys.size() - 2;
}

std::size_t FindQuatKeyIndex(
    const std::vector<aiQuatKey>& keys,
    double time_ticks)
{
    if (keys.size() < 2)
    {
        return 0;
    }
    for (std::size_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (time_ticks < keys[i + 1].mTime)
        {
            return i;
        }
    }
    return keys.size() - 2;
}

aiVector3D InterpolateVectorKeys(
    const std::vector<aiVectorKey>& keys,
    double time_ticks)
{
    if (keys.empty())
    {
        return aiVector3D(0.0f, 0.0f, 0.0f);
    }
    if (keys.size() == 1)
    {
        return keys.front().mValue;
    }
    const std::size_t index = FindVectorKeyIndex(keys, time_ticks);
    const std::size_t next_index = std::min(index + 1, keys.size() - 1);
    const double start_time = keys[index].mTime;
    const double end_time = keys[next_index].mTime;
    if (end_time <= start_time)
    {
        return keys[index].mValue;
    }
    const float factor = static_cast<float>(
        (time_ticks - start_time) / (end_time - start_time));
    return keys[index].mValue + (keys[next_index].mValue - keys[index].mValue) * factor;
}

aiQuaternion InterpolateQuatKeys(
    const std::vector<aiQuatKey>& keys,
    double time_ticks)
{
    if (keys.empty())
    {
        return aiQuaternion();
    }
    if (keys.size() == 1)
    {
        return keys.front().mValue;
    }
    const std::size_t index = FindQuatKeyIndex(keys, time_ticks);
    const std::size_t next_index = std::min(index + 1, keys.size() - 1);
    const double start_time = keys[index].mTime;
    const double end_time = keys[next_index].mTime;
    if (end_time <= start_time)
    {
        return keys[index].mValue;
    }
    const float factor = static_cast<float>(
        (time_ticks - start_time) / (end_time - start_time));
    aiQuaternion result;
    aiQuaternion::Interpolate(result, keys[index].mValue, keys[next_index].mValue, factor);
    result.Normalize();
    return result;
}

aiMatrix4x4 ComposeTransform(
    const aiVector3D& translation,
    const aiQuaternion& rotation,
    const aiVector3D& scaling)
{
    aiMatrix4x4 scale_matrix;
    aiMatrix4x4::Scaling(scaling, scale_matrix);
    aiMatrix4x4 rotation_matrix = aiMatrix4x4(rotation.GetMatrix());
    aiMatrix4x4 translation_matrix;
    aiMatrix4x4::Translation(translation, translation_matrix);
    return translation_matrix * rotation_matrix * scale_matrix;
}

void EvaluateNodeTransformsRecursive(
    const SkinAnimationData& animation_data,
    int node_index,
    const aiMatrix4x4& parent_transform,
    double time_ticks,
    std::vector<aiMatrix4x4>& node_globals,
    const AnimationClipData* clip_data)
{
    aiMatrix4x4 node_local = animation_data.nodes[node_index].bind_local_transform;
    const NodeAnimationChannel* channel_ptr = nullptr;
    if (clip_data)
    {
        if (auto channel_it = clip_data->channels.find(node_index);
            channel_it != clip_data->channels.end())
        {
            channel_ptr = &channel_it->second;
        }
    }
    if (channel_ptr)
    {
        const auto& channel = *channel_ptr;
        aiVector3D bind_scaling;
        aiQuaternion bind_rotation;
        aiVector3D bind_translation;
        node_local.Decompose(bind_scaling, bind_rotation, bind_translation);

        const aiVector3D translation = channel.position_keys.empty()
                                           ? bind_translation
                                           : InterpolateVectorKeys(channel.position_keys, time_ticks);
        const aiQuaternion rotation = channel.rotation_keys.empty()
                                          ? bind_rotation
                                          : InterpolateQuatKeys(channel.rotation_keys, time_ticks);
        const aiVector3D scaling = channel.scaling_keys.empty()
                                       ? bind_scaling
                                       : InterpolateVectorKeys(channel.scaling_keys, time_ticks);
        node_local = ComposeTransform(translation, rotation, scaling);
    }
    const aiMatrix4x4 node_global = parent_transform * node_local;
    node_globals[node_index] = node_global;
    for (const int child_index : animation_data.nodes[node_index].children)
    {
        EvaluateNodeTransformsRecursive(
            animation_data,
            child_index,
            node_global,
            time_ticks,
            node_globals,
            clip_data);
    }
}

std::vector<glm::mat4> EvaluateBoneMatrices(
    const std::shared_ptr<SkinAnimationData>& animation_data,
    double time_seconds,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index)
{
    if (!animation_data || animation_data->bones.empty() || animation_data->nodes.empty())
    {
        return {};
    }
    const AnimationClipData* clip_data =
        SelectAnimationClip(*animation_data, clip_name, clip_index);

    double time_ticks = 0.0;
    if (clip_data && clip_data->has_animation && clip_data->duration_ticks > 0.0)
    {
        const double ticks_per_second =
            clip_data->ticks_per_second > 0.0 ? clip_data->ticks_per_second : 25.0;
        time_ticks = std::fmod(time_seconds * ticks_per_second, clip_data->duration_ticks);
        if (time_ticks < 0.0)
        {
            time_ticks += clip_data->duration_ticks;
        }
    }

    std::vector<aiMatrix4x4> node_globals(animation_data->nodes.size(), aiMatrix4x4());
    EvaluateNodeTransformsRecursive(
        *animation_data,
        0,
        aiMatrix4x4(),
        time_ticks,
        node_globals,
        clip_data);

    std::vector<glm::mat4> bone_matrices(animation_data->bones.size(), glm::mat4(1.0f));
    for (std::size_t bone_index = 0; bone_index < animation_data->bones.size(); ++bone_index)
    {
        const auto& bone = animation_data->bones[bone_index];
        if (bone.node_index < 0 ||
            bone.node_index >= static_cast<int>(node_globals.size()))
        {
            continue;
        }
        const aiMatrix4x4 final_transform =
            animation_data->global_inverse_transform *
            node_globals[bone.node_index] *
            bone.offset_matrix;
        bone_matrices[bone_index] = AiToGlm(final_transform);
    }
    return bone_matrices;
}

bool EvaluateSkinnedVertices(
    const std::shared_ptr<SkinAnimationData>& skin_animation_data,
    double time_seconds,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index,
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<int>& bone_indices_flat,
    const std::vector<float>& bone_weights_flat,
    std::vector<float>& skinned_points,
    std::vector<float>& skinned_normals)
{
    const std::size_t vertex_count = points.size() / 3;
    if (!skin_animation_data ||
        vertex_count == 0 ||
        bone_indices_flat.size() < vertex_count * 4 ||
        bone_weights_flat.size() < vertex_count * 4)
    {
        return false;
    }

    auto bone_matrices = EvaluateBoneMatrices(
        skin_animation_data,
        time_seconds,
        clip_name,
        clip_index);
    if (bone_matrices.empty())
    {
        return false;
    }

    skinned_points.assign(points.size(), 0.0f);
    skinned_normals.assign(normals.size(), 0.0f);
    for (std::size_t vertex = 0; vertex < vertex_count; ++vertex)
    {
        const std::size_t offset = vertex * 3;
        const std::size_t bone_offset = vertex * 4;
        glm::mat4 skin_matrix(0.0f);
        float weight_sum = 0.0f;
        for (std::size_t slot = 0; slot < 4; ++slot)
        {
            const float weight = bone_weights_flat[bone_offset + slot];
            if (weight <= 0.0f)
            {
                continue;
            }
            const int bone_index = bone_indices_flat[bone_offset + slot];
            if (bone_index < 0 ||
                bone_index >= static_cast<int>(bone_matrices.size()))
            {
                continue;
            }
            skin_matrix += bone_matrices[bone_index] * weight;
            weight_sum += weight;
        }
        if (weight_sum <= 0.0f)
        {
            skin_matrix = glm::mat4(1.0f);
        }

        const glm::vec4 point =
            skin_matrix *
            glm::vec4(
                points[offset],
                points[offset + 1],
                points[offset + 2],
                1.0f);
        skinned_points[offset] = point.x;
        skinned_points[offset + 1] = point.y;
        skinned_points[offset + 2] = point.z;

        if (offset + 2 < normals.size())
        {
            glm::vec3 normal = glm::mat3(skin_matrix) * glm::vec3(
                                   normals[offset],
                                   normals[offset + 1],
                                   normals[offset + 2]);
            if (glm::length(normal) > 1.0e-6f)
            {
                normal = glm::normalize(normal);
            }
            skinned_normals[offset] = normal.x;
            skinned_normals[offset + 1] = normal.y;
            skinned_normals[offset + 2] = normal.z;
        }
    }
    return true;
}

std::vector<float> BuildRaytraceTriangles(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::uint32_t>& indices)
{
    std::vector<float> triangles = {};
    triangles.reserve(indices.size() * 16);
    auto push_vertex = [&](std::uint32_t index) {
        const auto base = index * 3;
        const auto uv_base = index * 2;
        triangles.push_back(points[base + 0]);
        triangles.push_back(points[base + 1]);
        triangles.push_back(points[base + 2]);
        triangles.push_back(0.0f);
        if (!normals.empty())
        {
            triangles.push_back(normals[base + 0]);
            triangles.push_back(normals[base + 1]);
            triangles.push_back(normals[base + 2]);
        }
        else
        {
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
        }
        triangles.push_back(0.0f);
        if (!textures.empty())
        {
            triangles.push_back(textures[uv_base + 0]);
            triangles.push_back(textures[uv_base + 1]);
        }
        else
        {
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
        }
        triangles.push_back(0.0f);
        triangles.push_back(0.0f);
    };
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        push_vertex(indices[i]);
        push_vertex(indices[i + 1]);
        push_vertex(indices[i + 2]);
    }
    return triangles;
}

std::vector<float> EvaluateSkinnedTriangles(
    const std::shared_ptr<SkinAnimationData>& skin_animation_data,
    double time_seconds,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index,
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::uint32_t>& indices,
    const std::vector<int>& bone_indices_flat,
    const std::vector<float>& bone_weights_flat)
{
    std::vector<float> skinned_points = {};
    std::vector<float> skinned_normals = {};
    if (!EvaluateSkinnedVertices(
            skin_animation_data,
            time_seconds,
            clip_name,
            clip_index,
            points,
            normals,
            bone_indices_flat,
            bone_weights_flat,
            skinned_points,
            skinned_normals))
    {
        return BuildRaytraceTriangles(points, normals, textures, indices);
    }
    return BuildRaytraceTriangles(
        skinned_points,
        skinned_normals.empty() ? normals : skinned_normals,
        textures,
        indices);
}

std::vector<frame::BVHNode> EvaluateSkinnedBvh(
    const std::shared_ptr<SkinAnimationData>& skin_animation_data,
    double time_seconds,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index,
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<std::uint32_t>& indices,
    const std::vector<int>& bone_indices_flat,
    const std::vector<float>& bone_weights_flat)
{
    std::vector<float> skinned_points = {};
    std::vector<float> skinned_normals = {};
    if (!EvaluateSkinnedVertices(
            skin_animation_data,
            time_seconds,
            clip_name,
            clip_index,
            points,
            normals,
            bone_indices_flat,
            bone_weights_flat,
            skinned_points,
            skinned_normals))
    {
        return frame::BuildBVH(points, indices);
    }
    return frame::BuildBVH(skinned_points, indices);
}

bool ParseNodeMatrix(
    LevelInterface& level,
    const frame::proto::NodeMatrix& proto_matrix)
{
    if (!proto_matrix.has_matrix_type_enum())
    {
        throw std::runtime_error(std::format(
            "Node matrix '{}' must explicitly set matrix_type_enum to STATIC_MATRIX or ROTATION_MATRIX.",
            proto_matrix.name()));
    }
    std::unique_ptr<frame::NodeMatrix> node_matrix;
    const bool rotation =
        proto_matrix.matrix_type_enum() ==
            frame::proto::NodeMatrix::ROTATION_MATRIX;

    if (proto_matrix.has_matrix())
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level),
            frame::json::ParseUniform(proto_matrix.matrix()),
            rotation);
    }
    else if (proto_matrix.has_quaternion())
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level),
            frame::json::ParseUniform(proto_matrix.quaternion()),
            rotation);
    }
    else
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level), glm::mat4(1.0f), rotation);
    }

    if (rotation)
    {
        node_matrix->GetData().set_matrix_type_enum(
            frame::proto::NodeMatrix::ROTATION_MATRIX);
    }

    node_matrix->GetData().set_name(proto_matrix.name());
    node_matrix->SetParentName(proto_matrix.parent());
    auto maybe_id = level.AddSceneNode(std::move(node_matrix));
    return static_cast<bool>(maybe_id);
}

bool ParseNodeMeshFromEnum(
    LevelInterface& level,
    const frame::proto::NodeMesh& proto_mesh)
{
    if (proto_mesh.mesh_enum() == frame::proto::NodeMesh::INVALID)
    {
        throw std::runtime_error("Static mesh enum is invalid.");
    }

    EntityId mesh_id = frame::NullId;
    switch (proto_mesh.mesh_enum())
    {
    case frame::proto::NodeMesh::CUBE:
        mesh_id = level.GetDefaultMeshCubeId();
        break;
    case frame::proto::NodeMesh::QUAD:
        mesh_id = level.GetDefaultMeshQuadId();
        break;
    default:
        throw std::runtime_error("Unsupported static mesh enum for Vulkan.");
    }

    if (!mesh_id)
    {
        throw std::runtime_error("Static mesh not available in level.");
    }

    auto material_id = CreateAutoMaterial(
        level,
        proto_mesh.name(),
        proto_mesh.render_time_enum());

    auto node = std::make_unique<frame::NodeMesh>(
        MakeResolver(level), mesh_id);
    node->GetData().set_name(proto_mesh.name());
    node->SetParentName(proto_mesh.parent());
    node->GetData().set_render_time_enum(proto_mesh.render_time_enum());
    node->GetData().set_acceleration_structure_enum(
        proto_mesh.acceleration_structure_enum());
    node->GetData().set_play_animation(proto_mesh.play_animation());
    if (proto_mesh.has_animation_speed())
    {
        node->GetData().set_animation_speed(proto_mesh.animation_speed());
    }
    ConfigureMaterialProgramsForRenderTime(
        level,
        material_id,
        proto_mesh.render_time_enum());
    if (proto_mesh.has_animation_clip_name())
    {
        node->GetData().set_animation_clip_name(proto_mesh.animation_clip_name());
    }
    if (proto_mesh.has_animation_clip_index())
    {
        node->GetData().set_animation_clip_index(proto_mesh.animation_clip_index());
    }

    auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(scene_id, material_id, proto_mesh.render_time_enum());
    return true;
}

bool ParseNodeMeshCleanBuffer(
    LevelInterface& level,
    const frame::proto::NodeMesh& proto_mesh)
{
    auto node = std::make_unique<frame::NodeMesh>(
        MakeResolver(level), proto_mesh.clean_buffer());
    node->GetData().set_name(proto_mesh.name());
    node->SetParentName(proto_mesh.parent());
    node->GetData().set_render_time_enum(proto_mesh.render_time_enum());
    node->GetData().set_acceleration_structure_enum(
        proto_mesh.acceleration_structure_enum());
    node->GetData().set_play_animation(proto_mesh.play_animation());
    if (proto_mesh.has_animation_speed())
    {
        node->GetData().set_animation_speed(proto_mesh.animation_speed());
    }
    if (proto_mesh.has_animation_clip_name())
    {
        node->GetData().set_animation_clip_name(proto_mesh.animation_clip_name());
    }
    if (proto_mesh.has_animation_clip_index())
    {
        node->GetData().set_animation_clip_index(proto_mesh.animation_clip_index());
    }
    auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(
        scene_id, frame::NullId, proto_mesh.render_time_enum());
    return true;
}

bool ParseNodeMesh(
    LevelInterface& level,
    const frame::proto::NodeMesh& proto_mesh,
    const ParseSceneTreeOptions& options)
{
    if (proto_mesh.has_clean_buffer())
    {
        return ParseNodeMeshCleanBuffer(level, proto_mesh);
    }
    if (proto_mesh.has_mesh_enum())
    {
        return ParseNodeMeshFromEnum(level, proto_mesh);
    }
    if (proto_mesh.has_file_name())
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        const auto path = (asset_root / "model" / proto_mesh.file_name())
                              .lexically_normal();
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (extension != ".glb" && extension != ".gltf")
        {
            throw std::runtime_error(std::format(
                "Unsupported mesh format for Vulkan JSON parser: {}. "
                "Only .glb/.gltf are supported.",
                path.string()));
        }
        Assimp::Importer importer;
        constexpr unsigned int kPostProcessFlags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_LimitBoneWeights |
            aiProcess_SortByPType;
        const aiScene* scene = importer.ReadFile(path.string(), kPostProcessFlags);
        if (!scene || !scene->HasMeshes())
        {
            throw std::runtime_error(
                std::format(
                    "Failed to load mesh file {}: {}",
                    path.string(),
                    importer.GetErrorString()));
        }
        std::unordered_map<unsigned int, aiMatrix4x4> mesh_transforms;
        if (scene->mRootNode)
        {
            GatherNodeMeshTransforms(
                scene->mRootNode, aiMatrix4x4(), mesh_transforms);
        }

        std::vector<SkinAnimationData::NodeData> scene_nodes = {};
        std::unordered_map<std::string, int> scene_node_indices = {};
        std::vector<AnimationClipData> scene_animation_clips = {};
        std::unordered_map<std::string, std::size_t> scene_clip_name_to_index = {};
        aiMatrix4x4 scene_global_inverse = aiMatrix4x4();
        if (scene->mRootNode)
        {
            BuildNodeHierarchy(
                scene->mRootNode,
                -1,
                scene_nodes,
                scene_node_indices);
            scene_global_inverse = scene->mRootNode->mTransformation;
            scene_global_inverse.Inverse();
        }
        scene_animation_clips.reserve(scene->mNumAnimations);
        for (unsigned int animation_index = 0;
             animation_index < scene->mNumAnimations;
             ++animation_index)
        {
            const aiAnimation* animation = scene->mAnimations[animation_index];
            if (!animation)
            {
                continue;
            }
            AnimationClipData clip_data;
            clip_data.name = animation->mName.C_Str();
            clip_data.duration_ticks = animation->mDuration;
            clip_data.ticks_per_second =
                animation->mTicksPerSecond > 0.0
                    ? animation->mTicksPerSecond
                    : 25.0;
            clip_data.has_animation =
                animation->mNumChannels > 0 && animation->mDuration > 0.0;
            for (unsigned int channel_index = 0;
                 channel_index < animation->mNumChannels;
                 ++channel_index)
            {
                const aiNodeAnim* channel = animation->mChannels[channel_index];
                auto node_it =
                    scene_node_indices.find(channel->mNodeName.C_Str());
                if (node_it == scene_node_indices.end())
                {
                    continue;
                }
                NodeAnimationChannel channel_copy;
                channel_copy.position_keys.assign(
                    channel->mPositionKeys,
                    channel->mPositionKeys + channel->mNumPositionKeys);
                channel_copy.rotation_keys.assign(
                    channel->mRotationKeys,
                    channel->mRotationKeys + channel->mNumRotationKeys);
                channel_copy.scaling_keys.assign(
                    channel->mScalingKeys,
                    channel->mScalingKeys + channel->mNumScalingKeys);
                clip_data.channels[node_it->second] = std::move(channel_copy);
            }
            const std::size_t clip_index = scene_animation_clips.size();
            if (!clip_data.name.empty())
            {
                const auto clip_key = ToLowerAscii(clip_data.name);
                if (!scene_clip_name_to_index.contains(clip_key))
                {
                    scene_clip_name_to_index[clip_key] = clip_index;
                }
            }
            scene_animation_clips.push_back(std::move(clip_data));
        }

        const EntityId selected_program_id =
            level.GetRenderPassProgramId(proto_mesh.render_time_enum())
                ? level.GetRenderPassProgramId(proto_mesh.render_time_enum())
                : SelectGltfProgramId(level);
        ProgramInterface* selected_program =
            (selected_program_id != NullId)
                ? &level.GetProgramFromId(selected_program_id)
                : nullptr;
        const bool selected_program_is_raytracing =
            selected_program &&
            frame::json::IsRaytracingProgramKey(
                frame::json::ResolveProgramKey(selected_program->GetData()));
        const bool use_hardware_raytracing_path =
            options.prefer_hardware_raytracing &&
            selected_program_is_raytracing;
        const bool build_bvh =
            !use_hardware_raytracing_path &&
            (selected_program_is_raytracing ||
             proto_mesh.acceleration_structure_enum() ==
                 frame::proto::NodeMesh::BVH_ACCELERATION);
        const glm::uvec2 texture_display_size = ResolveTextureDisplaySize(level);
        std::unordered_map<std::string, EntityId> file_texture_cache = {};
        std::unordered_map<std::string, EntityId> solid_texture_cache = {};
        std::unordered_map<unsigned int, EntityId> gltf_material_cache = {};
        int generated_texture_counter = 0;
        int generated_material_counter = 0;

        auto bytes_per_component =
            [](const frame::proto::PixelElementSize& element_size) {
                switch (element_size.value())
                {
                case frame::proto::PixelElementSize::BYTE:
                    return sizeof(std::uint8_t);
                case frame::proto::PixelElementSize::SHORT:
                case frame::proto::PixelElementSize::HALF:
                    return sizeof(std::uint16_t);
                case frame::proto::PixelElementSize::FLOAT:
                    return sizeof(float);
                default:
                    throw std::runtime_error("Unsupported texture element size.");
                }
            };
        auto component_count =
            [](const frame::proto::PixelStructure& pixel_structure) {
                switch (pixel_structure.value())
                {
                case frame::proto::PixelStructure::GREY:
                case frame::proto::PixelStructure::DEPTH:
                    return 1u;
                case frame::proto::PixelStructure::GREY_ALPHA:
                    return 2u;
                case frame::proto::PixelStructure::RGB:
                case frame::proto::PixelStructure::BGR:
                    return 3u;
                case frame::proto::PixelStructure::RGB_ALPHA:
                case frame::proto::PixelStructure::BGR_ALPHA:
                    return 4u;
                default:
                    throw std::runtime_error(
                        "Unsupported texture pixel structure.");
                }
            };
        auto create_texture_from_inline_pixels =
            [&](const void* pixels,
                glm::uvec2 size,
                const frame::proto::PixelElementSize& element_size,
                const frame::proto::PixelStructure& pixel_structure,
                const std::string& cache_key,
                const std::string& semantic) -> EntityId {
                if (auto it = file_texture_cache.find(cache_key);
                    it != file_texture_cache.end())
                {
                    return it->second;
                }
                frame::proto::Texture proto_texture;
                const std::string texture_name = std::format(
                    "{}.__gltf_tex_{}_{}",
                    proto_mesh.name(),
                    semantic,
                    generated_texture_counter++);
                proto_texture.set_name(texture_name);
                proto_texture.mutable_pixel_element_size()->CopyFrom(
                    element_size);
                proto_texture.mutable_pixel_structure()->CopyFrom(
                    pixel_structure);
                proto_texture.mutable_size()->set_x(static_cast<int>(size.x));
                proto_texture.mutable_size()->set_y(static_cast<int>(size.y));
                const std::size_t pixel_bytes =
                    static_cast<std::size_t>(size.x) * size.y *
                    component_count(pixel_structure) *
                    bytes_per_component(element_size);
                proto_texture.set_pixels(
                    reinterpret_cast<const char*>(pixels),
                    static_cast<int>(pixel_bytes));
                auto texture =
                    frame::vulkan::json::ParseTexture(
                        proto_texture, texture_display_size);
                texture->SetName(texture_name);
                texture->SetSerializeEnable(true);
                EntityId texture_id = level.AddTexture(std::move(texture));
                file_texture_cache.emplace(cache_key, texture_id);
                return texture_id;
            };

        auto create_texture_from_path =
            [&](const std::filesystem::path& path,
                const std::string& semantic) -> EntityId {
            const auto normalized =
                std::filesystem::absolute(path).lexically_normal();
            const std::string cache_key =
                std::format("{}|{}", semantic, normalized.generic_string());
            if (auto it = file_texture_cache.find(cache_key);
                it != file_texture_cache.end())
            {
                return it->second;
            }
            frame::proto::Texture proto_texture;
            const std::string texture_name = std::format(
                "{}.__gltf_tex_{}_{}",
                proto_mesh.name(),
                semantic,
                generated_texture_counter++);
            proto_texture.set_name(texture_name);
            proto_texture.mutable_pixel_element_size()->CopyFrom(
                frame::json::PixelElementSize_BYTE());
            proto_texture.mutable_pixel_structure()->CopyFrom(
                frame::json::PixelStructure_RGB_ALPHA());
            proto_texture.set_file_name(frame::file::PurifyFilePath(normalized));
            auto texture =
                frame::vulkan::json::ParseTexture(
                    proto_texture, texture_display_size);
            texture->SetName(texture_name);
            texture->SetSerializeEnable(true);
            EntityId texture_id = level.AddTexture(std::move(texture));
            file_texture_cache.emplace(cache_key, texture_id);
            return texture_id;
        };
        auto create_texture_from_embedded =
            [&](const aiTexture& embedded_texture,
                const std::string& source_key,
                const std::string& semantic) -> EntityId {
                const std::string cache_key =
                    std::format("{}|{}", semantic, source_key);
                if (embedded_texture.mHeight == 0)
                {
                    const frame::file::Image image(
                        embedded_texture.pcData,
                        embedded_texture.mWidth,
                        frame::json::PixelElementSize_BYTE(),
                        frame::json::PixelStructure_RGB_ALPHA());
                    return create_texture_from_inline_pixels(
                        image.Data(),
                        image.GetSize(),
                        image.GetPixelElementSize(),
                        image.GetPixelStructure(),
                        cache_key,
                        semantic);
                }

                std::vector<std::uint8_t> rgba_pixels;
                rgba_pixels.reserve(
                    static_cast<std::size_t>(embedded_texture.mWidth) *
                    embedded_texture.mHeight * 4);
                for (unsigned int i = 0;
                     i < embedded_texture.mWidth * embedded_texture.mHeight;
                     ++i)
                {
                    const auto& texel = embedded_texture.pcData[i];
                    rgba_pixels.push_back(texel.r);
                    rgba_pixels.push_back(texel.g);
                    rgba_pixels.push_back(texel.b);
                    rgba_pixels.push_back(texel.a);
                }
                return create_texture_from_inline_pixels(
                    rgba_pixels.data(),
                    {embedded_texture.mWidth, embedded_texture.mHeight},
                    frame::json::PixelElementSize_BYTE(),
                    frame::json::PixelStructure_RGB_ALPHA(),
                    cache_key,
                    semantic);
            };
        auto create_texture_from_source =
            [&](const MaterialTextureSource& source,
                const std::string& semantic) -> EntityId {
                if (source.file_path)
                {
                    return create_texture_from_path(*source.file_path, semantic);
                }
                if (source.embedded_texture)
                {
                    return create_texture_from_embedded(
                        *source.embedded_texture,
                        source.cache_key,
                        semantic);
                }
                return NullId;
            };

        auto create_solid_texture =
            [&](const glm::vec4& color,
                const std::string& semantic,
                const frame::proto::PixelElementSize& element_size =
                    frame::json::PixelElementSize_BYTE()) -> EntityId {
            const std::string cache_key = std::format(
                "{}|{}|{}|{}|{}|{}",
                semantic,
                color.x,
                color.y,
                color.z,
                color.w,
                static_cast<int>(element_size.value()));
            if (auto it = solid_texture_cache.find(cache_key);
                it != solid_texture_cache.end())
            {
                return it->second;
            }
            frame::proto::Texture proto_texture;
            const std::string texture_name = std::format(
                "{}.__gltf_solid_{}_{}",
                proto_mesh.name(),
                semantic,
                generated_texture_counter++);
            proto_texture.set_name(texture_name);
            proto_texture.mutable_pixel_element_size()->CopyFrom(element_size);
            proto_texture.mutable_pixel_structure()->CopyFrom(
                frame::json::PixelStructure_RGB_ALPHA());
            proto_texture.mutable_size()->set_x(1);
            proto_texture.mutable_size()->set_y(1);
            if (element_size.value() == frame::proto::PixelElementSize::FLOAT)
            {
                std::array<float, 4> pixels = {
                    color.x,
                    color.y,
                    color.z,
                    color.w};
                proto_texture.set_pixels(
                    reinterpret_cast<const char*>(pixels.data()),
                    pixels.size() * sizeof(float));
            }
            else
            {
                std::array<std::uint8_t, 4> pixels = {
                    static_cast<std::uint8_t>(
                        std::clamp(color.x, 0.0f, 1.0f) * 255.0f),
                    static_cast<std::uint8_t>(
                        std::clamp(color.y, 0.0f, 1.0f) * 255.0f),
                    static_cast<std::uint8_t>(
                        std::clamp(color.z, 0.0f, 1.0f) * 255.0f),
                    static_cast<std::uint8_t>(
                        std::clamp(color.w, 0.0f, 1.0f) * 255.0f)};
                proto_texture.set_pixels(
                    reinterpret_cast<const char*>(pixels.data()), pixels.size());
            }
            auto texture =
                frame::vulkan::json::ParseTexture(
                    proto_texture, texture_display_size);
            texture->SetName(texture_name);
            texture->SetSerializeEnable(true);
            EntityId texture_id = level.AddTexture(std::move(texture));
            solid_texture_cache.emplace(cache_key, texture_id);
            return texture_id;
        };

        auto find_level_texture =
            [&](std::initializer_list<std::string_view> names) -> EntityId {
                for (const auto name_view : names)
                {
                    const auto texture_id = level.GetIdFromName(
                        std::string(name_view));
                    if (texture_id != NullId)
                    {
                        return texture_id;
                    }
                }
                return NullId;
            };

        auto create_material_from_gltf = [&](unsigned int material_index) -> EntityId {
            if (selected_program_id == NullId ||
                material_index >= scene->mNumMaterials)
            {
                return NullId;
            }
            if (auto it = gltf_material_cache.find(material_index);
                it != gltf_material_cache.end())
            {
                return it->second;
            }

            const aiMaterial* ai_material = scene->mMaterials[material_index];
            const auto base_color_texture = FindFirstMaterialTextureSource(
                scene,
                ai_material,
                path,
                {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE});
            const auto normal_texture = FindFirstMaterialTextureSource(
                scene,
                ai_material,
                path,
                {aiTextureType_NORMALS, aiTextureType_HEIGHT});
            const auto roughness_texture = FindFirstMaterialTextureSource(
                scene,
                ai_material,
                path,
                {aiTextureType_DIFFUSE_ROUGHNESS});
            const auto metallic_texture = FindFirstMaterialTextureSource(
                scene,
                ai_material,
                path,
                {aiTextureType_METALNESS});
            const auto ao_texture = FindFirstMaterialTextureSource(
                scene,
                ai_material,
                path,
                {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP});
            const auto specular_texture = FindMaterialTextureSource(
                scene,
                ai_material,
                path,
                aiTextureType_SPECULAR,
                0);
            const auto transmission_texture = FindMaterialTextureSource(
                scene,
                ai_material,
                path,
                aiTextureType_TRANSMISSION,
                0);
            const auto thickness_texture = FindMaterialTextureSource(
                scene,
                ai_material,
                path,
                aiTextureType_TRANSMISSION,
                1);

            const glm::vec4 base_color_factor = ReadBaseColorFactor(ai_material);
            const float roughness_factor = ReadRoughnessFactor(ai_material);
            const float metallic_factor = ReadMetallicFactor(ai_material);
            const float specular_factor = ReadSpecularFactor(ai_material);
            const glm::vec3 specular_color = ReadSpecularColor(ai_material);
            const float transmission_factor = ReadTransmissionFactor(ai_material);
            const float ior_factor = ReadIorFactor(ai_material);
            const float thickness_factor = ReadThicknessFactor(ai_material);
            const float attenuation_distance =
                ReadAttenuationDistance(ai_material);
            const glm::vec3 attenuation_color =
                ReadAttenuationColor(ai_material);
            const bool prefer_scene_fallback_textures =
                transmission_factor <= 0.01f && !transmission_texture.has_value();

            auto use_scene_texture_or =
                [&](std::initializer_list<std::string_view> names,
                    EntityId fallback_texture_id) -> EntityId {
                    if (!prefer_scene_fallback_textures)
                    {
                        return fallback_texture_id;
                    }
                    const auto texture_id = find_level_texture(names);
                    return texture_id != NullId ? texture_id : fallback_texture_id;
                };

            const EntityId base_texture_id = base_color_texture
                ? create_texture_from_source(*base_color_texture, "base_color")
                : use_scene_texture_or(
                      {"albedo_texture", "Color"},
                      create_solid_texture(base_color_factor, "base_color"));
            const EntityId normal_texture_id = normal_texture
                ? create_texture_from_source(*normal_texture, "normal")
                : use_scene_texture_or(
                      {"normal_texture"},
                      create_solid_texture(
                          glm::vec4(0.5f, 0.5f, 1.0f, 1.0f),
                          "normal"));
            const EntityId roughness_texture_id = roughness_texture
                ? create_texture_from_source(*roughness_texture, "roughness")
                : use_scene_texture_or(
                      {"roughness_texture"},
                      create_solid_texture(
                          glm::vec4(
                              roughness_factor,
                              roughness_factor,
                              roughness_factor,
                              1.0f),
                          "roughness"));
            const EntityId metallic_texture_id = metallic_texture
                ? create_texture_from_source(*metallic_texture, "metallic")
                : use_scene_texture_or(
                      {"metallic_texture"},
                      create_solid_texture(
                          glm::vec4(
                              metallic_factor,
                              metallic_factor,
                              metallic_factor,
                              1.0f),
                          "metallic"));
            const EntityId ao_texture_id = ao_texture
                ? create_texture_from_source(*ao_texture, "ao")
                : use_scene_texture_or(
                      {"ao_texture"},
                      create_solid_texture(glm::vec4(1.0f), "ao"));
            const EntityId specular_color_texture_id = specular_texture
                ? create_texture_from_source(*specular_texture, "specular")
                : use_scene_texture_or(
                      {"specular_color_texture"},
                      create_solid_texture(
                          glm::vec4(specular_color, 1.0f),
                          "specular_color"));
            const EntityId specular_factor_texture_id = specular_texture
                ? specular_color_texture_id
                : use_scene_texture_or(
                      {"specular_factor_texture"},
                      create_solid_texture(
                          glm::vec4(
                              specular_factor,
                              specular_factor,
                              specular_factor,
                              specular_factor),
                          "specular_factor"));
            const EntityId transmission_texture_id = transmission_texture
                ? create_texture_from_source(*transmission_texture, "transmission")
                : use_scene_texture_or(
                      {"transmission_texture"},
                      create_solid_texture(
                          glm::vec4(
                              transmission_factor,
                              transmission_factor,
                              transmission_factor,
                              1.0f),
                          "transmission"));
            const EntityId ior_texture_id = use_scene_texture_or(
                {"ior_texture"},
                create_solid_texture(
                    glm::vec4(ior_factor, ior_factor, ior_factor, 1.0f),
                    "ior",
                    frame::json::PixelElementSize_FLOAT()));
            const EntityId thickness_texture_id = thickness_texture
                ? create_texture_from_source(*thickness_texture, "thickness")
                : use_scene_texture_or(
                      {"thickness_texture"},
                      create_solid_texture(
                          glm::vec4(
                              thickness_factor,
                              thickness_factor,
                              thickness_factor,
                              1.0f),
                          "thickness",
                          frame::json::PixelElementSize_FLOAT()));
            const EntityId attenuation_color_texture_id = use_scene_texture_or(
                {"attenuation_color_texture"},
                create_solid_texture(
                    glm::vec4(attenuation_color, 1.0f),
                    "attenuation_color"));
            const EntityId attenuation_distance_texture_id =
                use_scene_texture_or(
                    {"attenuation_distance_texture"},
                    create_solid_texture(
                        glm::vec4(
                            attenuation_distance,
                            attenuation_distance,
                            attenuation_distance,
                            1.0f),
                        "attenuation_distance",
                        frame::json::PixelElementSize_FLOAT()));

            auto material = std::make_unique<frame::vulkan::Material>();
            const std::string material_name = std::format(
                "{}.__gltf_material_{}",
                proto_mesh.name(),
                generated_material_counter++);
            material->SetName(material_name);
            material->SetSerializeEnable(true);
            material->SetProgramId(selected_program_id);
            if (selected_program_is_raytracing)
            {
                const auto preprocess_id =
                    level.GetIdFromName("RayTracePreprocessProgram");
                if (preprocess_id != NullId)
                {
                    material->SetPreprocessProgramId(preprocess_id);
                }
                material->AddTextureId(
                    base_texture_id, "albedo_texture");
                material->AddTextureId(
                    normal_texture_id, "normal_texture");
                material->AddTextureId(
                    roughness_texture_id, "roughness_texture");
                material->AddTextureId(
                    metallic_texture_id, "metallic_texture");
                material->AddTextureId(ao_texture_id, "ao_texture");
                material->AddTextureId(
                    specular_factor_texture_id, "specular_factor_texture");
                material->AddTextureId(
                    specular_color_texture_id, "specular_color_texture");
                material->AddTextureId(
                    transmission_texture_id, "transmission_texture");
                material->AddTextureId(ior_texture_id, "ior_texture");
                material->AddTextureId(
                    thickness_texture_id, "thickness_texture");
                material->AddTextureId(
                    attenuation_color_texture_id,
                    "attenuation_color_texture");
                material->AddTextureId(
                    attenuation_distance_texture_id,
                    "attenuation_distance_texture");
            }
            else
            {
                const auto& program = level.GetProgramFromId(selected_program_id);
                for (const auto& binding : program.GetData().bindings())
                {
                    if (binding.binding_type() !=
                        frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER)
                    {
                        continue;
                    }
                    EntityId texture_id = NullId;
                    const std::string& binding_name = binding.name();
                    if (binding_name == "Color" ||
                        binding_name == "albedo_texture")
                    {
                        texture_id = base_texture_id;
                    }
                    else if (binding_name == "normal_texture")
                    {
                        texture_id = normal_texture_id;
                    }
                    else if (binding_name == "roughness_texture")
                    {
                        texture_id = roughness_texture_id;
                    }
                    else if (binding_name == "metallic_texture")
                    {
                        texture_id = metallic_texture_id;
                    }
                    else if (binding_name == "ao_texture")
                    {
                        texture_id = ao_texture_id;
                    }
                    else if (binding_name == "specular_factor_texture")
                    {
                        texture_id = specular_factor_texture_id;
                    }
                    else if (binding_name == "specular_color_texture")
                    {
                        texture_id = specular_color_texture_id;
                    }
                    else if (binding_name == "transmission_texture")
                    {
                        texture_id = transmission_texture_id;
                    }
                    else if (binding_name == "ior_texture")
                    {
                        texture_id = ior_texture_id;
                    }
                    else if (binding_name == "thickness_texture")
                    {
                        texture_id = thickness_texture_id;
                    }
                    else if (binding_name == "attenuation_color_texture")
                    {
                        texture_id = attenuation_color_texture_id;
                    }
                    else if (binding_name == "attenuation_distance_texture")
                    {
                        texture_id = attenuation_distance_texture_id;
                    }
                    else
                    {
                        texture_id = level.GetIdFromName(binding_name);
                    }
                    if (texture_id == NullId)
                    {
                        continue;
                    }
                    material->AddTextureId(texture_id, binding_name);
                }
            }

            const EntityId material_id = level.AddMaterial(std::move(material));
            gltf_material_cache.emplace(material_index, material_id);
            return material_id;
        };
        int counter = 0;
        for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
        {
            const aiMesh* mesh = scene->mMeshes[mesh_index];
            if (!mesh || mesh->mNumVertices == 0)
            {
                continue;
            }
            EntityId gltf_material_id = NullId;
            if (mesh->mMaterialIndex < scene->mNumMaterials)
            {
                gltf_material_id =
                    create_material_from_gltf(mesh->mMaterialIndex);
            }
            EntityId material_id = gltf_material_id;
            if (!material_id)
            {
                material_id = CreateAutoMaterial(
                    level,
                    std::format("{}.{}", proto_mesh.name(), counter),
                    proto_mesh.render_time_enum());
            }
            ConfigureMaterialProgramsForRenderTime(
                level,
                material_id,
                proto_mesh.render_time_enum());

            aiMatrix4x4 mesh_transform = aiMatrix4x4();
            auto transform_it = mesh_transforms.find(mesh_index);
            if (transform_it != mesh_transforms.end())
            {
                mesh_transform = transform_it->second;
            }
            aiMatrix3x3 normal_transform(mesh_transform);
            normal_transform.Inverse().Transpose();

            std::vector<float> points;
            std::vector<float> normals;
            std::vector<float> textures;
            points.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 3);
            normals.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 3);
            textures.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 2);
            const bool has_normals = mesh->HasNormals();
            const bool has_texcoords = mesh->HasTextureCoords(0);
            aiVector3D generated_uv_min(
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());
            aiVector3D generated_uv_max(
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest());
            bool generated_uv_valid = false;
            if (!has_texcoords)
            {
                for (unsigned int vertex_index = 0;
                     vertex_index < mesh->mNumVertices;
                     ++vertex_index)
                {
                    const aiVector3D p =
                        mesh_transform * mesh->mVertices[vertex_index];
                    generated_uv_min.x = std::min(generated_uv_min.x, p.x);
                    generated_uv_min.y = std::min(generated_uv_min.y, p.y);
                    generated_uv_min.z = std::min(generated_uv_min.z, p.z);
                    generated_uv_max.x = std::max(generated_uv_max.x, p.x);
                    generated_uv_max.y = std::max(generated_uv_max.y, p.y);
                    generated_uv_max.z = std::max(generated_uv_max.z, p.z);
                    generated_uv_valid = true;
                }
            }
            const aiVector3D generated_uv_center = generated_uv_valid
                ? aiVector3D(
                      (generated_uv_min.x + generated_uv_max.x) * 0.5f,
                      (generated_uv_min.y + generated_uv_max.y) * 0.5f,
                      (generated_uv_min.z + generated_uv_max.z) * 0.5f)
                : aiVector3D(0.0f, 0.0f, 0.0f);
            auto generate_uv = [&](const aiVector3D& position) {
                constexpr float kPi = 3.14159265358979323846f;
                aiVector3D direction = position - generated_uv_center;
                if (direction.SquareLength() <= 1.0e-10f)
                {
                    direction = aiVector3D(0.0f, 1.0f, 0.0f);
                }
                direction.Normalize();
                const float u = 0.5f +
                    std::atan2(direction.z, direction.x) / (2.0f * kPi);
                const float v = 0.5f -
                    std::asin(std::clamp(direction.y, -1.0f, 1.0f)) / kPi;
                return std::pair<float, float>(u, v);
            };
            for (unsigned int vertex_index = 0;
                 vertex_index < mesh->mNumVertices;
                 ++vertex_index)
            {
                const aiVector3D p =
                    mesh_transform * mesh->mVertices[vertex_index];
                points.push_back(p.x);
                points.push_back(p.y);
                points.push_back(p.z);
                if (has_normals)
                {
                    aiVector3D n =
                        normal_transform * mesh->mNormals[vertex_index];
                    n.Normalize();
                    normals.push_back(n.x);
                    normals.push_back(n.y);
                    normals.push_back(n.z);
                }
                else
                {
                    normals.push_back(0.0f);
                    normals.push_back(0.0f);
                    normals.push_back(1.0f);
                }

                if (has_texcoords)
                {
                    const aiVector3D& uv = mesh->mTextureCoords[0][vertex_index];
                    textures.push_back(std::isfinite(uv.x) ? uv.x : 0.0f);
                    textures.push_back(std::isfinite(uv.y) ? uv.y : 0.0f);
                }
                else
                {
                    const auto [u, v] = generate_uv(p);
                    textures.push_back(u);
                    textures.push_back(v);
                }
            }

            std::vector<std::uint32_t> indices;
            indices.reserve(static_cast<std::size_t>(mesh->mNumFaces) * 3);
            for (unsigned int face_index = 0;
                 face_index < mesh->mNumFaces;
                 ++face_index)
            {
                const aiFace& face = mesh->mFaces[face_index];
                if (face.mNumIndices < 3)
                {
                    continue;
                }
                for (unsigned int i = 0; i < face.mNumIndices; ++i)
                {
                    indices.push_back(face.mIndices[i]);
                }
            }
            const std::size_t vertex_count = points.size() / 3;
            const auto indices_valid =
                !indices.empty() &&
                std::all_of(
                    indices.begin(),
                    indices.end(),
                    [vertex_count](std::uint32_t idx) {
                        return idx < vertex_count;
                    }) &&
                (indices.size() % 3 == 0);
            std::vector<std::uint32_t> fallback_indices;
            const std::vector<std::uint32_t>* triangle_indices = &indices;
            if (!indices_valid && vertex_count > 0)
            {
                fallback_indices.resize(vertex_count);
                std::iota(fallback_indices.begin(), fallback_indices.end(), 0);
                const auto remainder = fallback_indices.size() % 3;
                if (remainder != 0)
                {
                    fallback_indices.resize(
                        fallback_indices.size() - remainder);
                }
                triangle_indices = &fallback_indices;
            }

            std::shared_ptr<SkinAnimationData> skin_animation_data = nullptr;
            std::vector<int> bone_indices_flat = {};
            std::vector<float> bone_weights_flat = {};
            if (mesh->HasBones())
            {
                constexpr std::size_t kMaxBones = 128;
                const std::size_t supported_bones =
                    std::min<std::size_t>(mesh->mNumBones, kMaxBones);
                skin_animation_data = std::make_shared<SkinAnimationData>();
                skin_animation_data->nodes = scene_nodes;
                skin_animation_data->node_indices = scene_node_indices;
                skin_animation_data->global_inverse_transform =
                    scene_global_inverse;
                skin_animation_data->bones.resize(supported_bones);
                skin_animation_data->clips = scene_animation_clips;
                skin_animation_data->clip_name_to_index =
                    scene_clip_name_to_index;

                std::vector<std::array<int, 4>> vertex_bone_indices(
                    mesh->mNumVertices,
                    std::array<int, 4>{0, 0, 0, 0});
                std::vector<std::array<float, 4>> vertex_bone_weights(
                    mesh->mNumVertices,
                    std::array<float, 4>{0.f, 0.f, 0.f, 0.f});

                for (std::size_t bone_index = 0;
                     bone_index < supported_bones;
                     ++bone_index)
                {
                    const aiBone* bone = mesh->mBones[bone_index];
                    auto node_it =
                        skin_animation_data->node_indices.find(bone->mName.C_Str());
                    if (node_it != skin_animation_data->node_indices.end())
                    {
                        skin_animation_data->bones[bone_index].node_index =
                            node_it->second;
                    }
                    else
                    {
                        skin_animation_data->bones[bone_index].node_index = -1;
                    }
                    skin_animation_data->bones[bone_index].offset_matrix =
                        bone->mOffsetMatrix;

                    for (unsigned int weight_index = 0;
                         weight_index < bone->mNumWeights;
                         ++weight_index)
                    {
                        const auto& weight = bone->mWeights[weight_index];
                        if (weight.mVertexId >= mesh->mNumVertices)
                        {
                            continue;
                        }
                        auto& ids = vertex_bone_indices[weight.mVertexId];
                        auto& weights = vertex_bone_weights[weight.mVertexId];

                        int target_slot = -1;
                        for (int slot = 0; slot < 4; ++slot)
                        {
                            if (weights[slot] == 0.0f)
                            {
                                target_slot = slot;
                                break;
                            }
                        }
                        if (target_slot < 0)
                        {
                            int weakest_slot = 0;
                            float weakest_weight = weights[0];
                            for (int slot = 1; slot < 4; ++slot)
                            {
                                if (weights[slot] < weakest_weight)
                                {
                                    weakest_weight = weights[slot];
                                    weakest_slot = slot;
                                }
                            }
                            if (weight.mWeight > weakest_weight)
                            {
                                target_slot = weakest_slot;
                            }
                        }
                        if (target_slot >= 0)
                        {
                            ids[target_slot] = static_cast<int>(bone_index);
                            weights[target_slot] = weight.mWeight;
                        }
                    }
                }

                bone_indices_flat.reserve(
                    static_cast<std::size_t>(mesh->mNumVertices) * 4);
                bone_weights_flat.reserve(
                    static_cast<std::size_t>(mesh->mNumVertices) * 4);
                for (unsigned int vertex_index = 0;
                     vertex_index < mesh->mNumVertices;
                     ++vertex_index)
                {
                    auto& weights = vertex_bone_weights[vertex_index];
                    const float sum =
                        weights[0] + weights[1] + weights[2] + weights[3];
                    if (sum > 0.0f)
                    {
                        for (float& value : weights)
                        {
                            value /= sum;
                        }
                    }
                    else
                    {
                        vertex_bone_indices[vertex_index] = {0, 0, 0, 0};
                        vertex_bone_weights[vertex_index] = {1.f, 0.f, 0.f, 0.f};
                    }

                    for (int slot = 0; slot < 4; ++slot)
                    {
                        bone_indices_flat.push_back(
                            vertex_bone_indices[vertex_index][slot]);
                        bone_weights_flat.push_back(
                            vertex_bone_weights[vertex_index][slot]);
                    }
                }
            }

            auto make_buffer = [](const auto& data,
                                  const std::string& name,
                                  LevelInterface& lvl) -> EntityId {
                if (data.empty())
                {
                    return NullId;
                }
                auto buffer = std::make_unique<frame::vulkan::Buffer>();
                buffer->Copy(data.size() * sizeof(data[0]), data.data());
                buffer->SetName(name);
                return lvl.AddBuffer(std::move(buffer));
            };

            auto point_buffer_id = make_buffer(
                points,
                std::format("{}.{}.point", proto_mesh.name(), counter),
                level);
            if (!point_buffer_id)
            {
                throw std::runtime_error("Failed to create point buffer.");
            }
            auto normal_buffer_id = make_buffer(
                normals,
                std::format("{}.{}.normal", proto_mesh.name(), counter),
                level);
            auto texture_buffer_id = make_buffer(
                textures,
                std::format("{}.{}.texture", proto_mesh.name(), counter),
                level);
            auto index_buffer_id = make_buffer(
                indices,
                std::format("{}.{}.index", proto_mesh.name(), counter),
                level);
            if (!index_buffer_id)
            {
                throw std::runtime_error("Failed to create index buffer.");
            }

            auto triangles = BuildRaytraceTriangles(
                points,
                normals,
                textures,
                *triangle_indices);

            auto triangle_buffer_id = make_buffer(
                triangles,
                std::format("{}.{}.triangle", proto_mesh.name(), counter),
                level);
            EntityId bvh_buffer_id = NullId;
            if (build_bvh)
            {
                auto bvh_nodes = frame::BuildBVH(points, *triangle_indices);
                bvh_buffer_id = make_buffer(
                    bvh_nodes,
                    std::format("{}.{}.bvh", proto_mesh.name(), counter),
                    level);
            }

            frame::MeshParameter parameter{};
            parameter.point_buffer_id = point_buffer_id;
            parameter.normal_buffer_id = normal_buffer_id;
            parameter.texture_buffer_id = texture_buffer_id;
            parameter.index_buffer_id = index_buffer_id;
            parameter.triangle_buffer_id = triangle_buffer_id;
            parameter.bvh_buffer_id = bvh_buffer_id;
            parameter.render_primitive_enum =
                proto_mesh.render_primitive_enum();

            std::unique_ptr<frame::MeshInterface> mesh_interface = nullptr;
            frame::vulkan::SkinnedMesh* skinned_mesh = nullptr;
            if (mesh->HasBones() &&
                skin_animation_data &&
                !bone_indices_flat.empty() &&
                !bone_weights_flat.empty())
            {
                auto skinned =
                    std::make_unique<frame::vulkan::SkinnedMesh>(parameter, true);
                skinned_mesh = skinned.get();
                mesh_interface = std::move(skinned);
            }
            else
            {
                mesh_interface =
                    std::make_unique<frame::vulkan::StaticMesh>(parameter, true);
            }

            mesh_interface->SetIndexSize(indices.size() * sizeof(std::uint32_t));
            const std::string mesh_name =
                scene->mNumMeshes == 1
                    ? std::format("{}.mesh", proto_mesh.name())
                    : std::format("{}.{}.mesh", proto_mesh.name(), counter);
            mesh_interface->SetName(mesh_name);
            mesh_interface->GetData().set_file_name(proto_mesh.file_name());
            mesh_interface->GetData().set_render_primitive_enum(
                proto_mesh.render_primitive_enum());
            mesh_interface->GetData().set_acceleration_structure_enum(
                proto_mesh.acceleration_structure_enum());
            if (skinned_mesh)
            {
                const float speed = proto_mesh.has_animation_speed()
                                        ? proto_mesh.animation_speed()
                                        : 1.0f;
                skinned_mesh->SetSkinningAnimation(
                    proto_mesh.play_animation(),
                    speed);
                const std::string clip_name =
                    proto_mesh.has_animation_clip_name()
                        ? proto_mesh.animation_clip_name()
                        : "";
                std::optional<std::uint32_t> clip_index = std::nullopt;
                if (proto_mesh.has_animation_clip_index())
                {
                    clip_index = proto_mesh.animation_clip_index();
                }
                skinned_mesh->SetSkinningAnimationClip(clip_name, clip_index);
                skinned_mesh->SetRaytraceTriangleCallback(
                    [skin_animation_data,
                     skinned_mesh,
                     points,
                     normals,
                     textures,
                     triangle_indices_copy = *triangle_indices,
                     bone_indices_flat,
                     bone_weights_flat](double time_seconds) {
                        if (!skinned_mesh)
                        {
                            return std::vector<float>{};
                        }
                        return EvaluateSkinnedTriangles(
                            skin_animation_data,
                            time_seconds,
                            skinned_mesh->GetSkinningAnimationClipName(),
                            skinned_mesh->GetSkinningAnimationClipIndex(),
                            points,
                            normals,
                            textures,
                            triangle_indices_copy,
                            bone_indices_flat,
                            bone_weights_flat);
                    });
                if (build_bvh)
                {
                    skinned_mesh->SetRaytraceBvhCallback(
                        [skin_animation_data,
                         skinned_mesh,
                         points,
                         normals,
                         triangle_indices_copy = *triangle_indices,
                         bone_indices_flat,
                         bone_weights_flat](double time_seconds) {
                            if (!skinned_mesh)
                            {
                                return std::vector<frame::BVHNode>{};
                            }
                            return EvaluateSkinnedBvh(
                                skin_animation_data,
                                time_seconds,
                                skinned_mesh->GetSkinningAnimationClipName(),
                                skinned_mesh->GetSkinningAnimationClipIndex(),
                                points,
                                normals,
                                triangle_indices_copy,
                                bone_indices_flat,
                                bone_weights_flat);
                        });
                }
            }

            auto mesh_id = level.AddMesh(std::move(mesh_interface));
            if (!mesh_id)
            {
                throw std::runtime_error("Failed to add static mesh to level.");
            }
            auto node = std::make_unique<frame::NodeMesh>(
                MakeResolver(level), mesh_id);
            std::string node_name = (scene->mNumMeshes == 1)
                                        ? proto_mesh.name()
                                        : std::format(
                                              "{}.{}",
                                              proto_mesh.name(),
                                              counter);
            node->SetName(node_name);
            node->SetParentName(proto_mesh.parent());
            node->GetData().set_render_time_enum(proto_mesh.render_time_enum());
            node->GetData().set_acceleration_structure_enum(
                proto_mesh.acceleration_structure_enum());
            node->GetData().set_file_name(proto_mesh.file_name());
            node->GetData().set_play_animation(proto_mesh.play_animation());
            if (proto_mesh.has_animation_speed())
            {
                node->GetData().set_animation_speed(proto_mesh.animation_speed());
            }
            if (proto_mesh.has_animation_clip_name())
            {
                node->GetData().set_animation_clip_name(
                    proto_mesh.animation_clip_name());
            }
            if (proto_mesh.has_animation_clip_index())
            {
                node->GetData().set_animation_clip_index(
                    proto_mesh.animation_clip_index());
            }

            auto scene_id = level.AddSceneNode(std::move(node));
            if (!material_id)
            {
                throw std::runtime_error(std::format(
                    "No material mapping found for mesh {} in file {}",
                    proto_mesh.name(),
                    proto_mesh.file_name()));
            }
            level.AddMeshMaterialId(
                scene_id, material_id, proto_mesh.render_time_enum());
            ++counter;
        }
        return true;
    }
    if (proto_mesh.has_multi_plugin())
    {
        throw std::runtime_error("Streamed static meshes are not implemented for Vulkan yet.");
    }
    return false;
}

bool ParseNodeCamera(
    LevelInterface& level,
    const frame::proto::NodeCamera& proto_camera)
{
    if (proto_camera.fov_degrees() == 0.0)
    {
        throw std::runtime_error("Camera must define a field of view.");
    }
    auto camera = std::make_unique<frame::NodeCamera>(
        MakeResolver(level),
        frame::json::ParseUniform(proto_camera.position()),
        frame::json::ParseUniform(proto_camera.target()),
        frame::json::ParseUniform(proto_camera.up()),
        proto_camera.fov_degrees(),
        proto_camera.aspect_ratio(),
        proto_camera.near_clip(),
        proto_camera.far_clip());
    camera->GetData().set_name(proto_camera.name());
    camera->SetParentName(proto_camera.parent());
    return static_cast<bool>(level.AddSceneNode(std::move(camera)));
}

bool ParseNodeLight(
    LevelInterface& level,
    const frame::proto::NodeLight& proto_light)
{
    switch (proto_light.light_type())
    {
    case frame::proto::NodeLight::POINT_LIGHT: {
        auto light = std::make_unique<frame::NodeLight>(
            MakeResolver(level),
            frame::LightTypeEnum::POINT_LIGHT,
            frame::json::ParseUniform(proto_light.position()),
            frame::json::ParseUniform(proto_light.color()));
        light->GetData().set_name(proto_light.name());
        light->SetParentName(proto_light.parent());
        light->GetData().set_shadow_type(proto_light.shadow_type());
        return static_cast<bool>(level.AddSceneNode(std::move(light)));
    }
    case frame::proto::NodeLight::DIRECTIONAL_LIGHT: {
        auto light = std::make_unique<frame::NodeLight>(
            MakeResolver(level),
            frame::LightTypeEnum::DIRECTIONAL_LIGHT,
            frame::json::ParseUniform(proto_light.direction()),
            frame::json::ParseUniform(proto_light.color()));
        light->GetData().set_name(proto_light.name());
        light->SetParentName(proto_light.parent());
        light->GetData().set_shadow_type(proto_light.shadow_type());
        return static_cast<bool>(level.AddSceneNode(std::move(light)));
    }
    default:
        throw std::runtime_error("Unsupported light type for Vulkan parser.");
    }
}

} // namespace

bool ParseSceneTree(
    const frame::proto::SceneTree& proto_scene_tree,
    LevelInterface& level,
    const ParseSceneTreeOptions& options)
{
    level.SetDefaultCameraName(proto_scene_tree.default_camera_name());
    level.SetDefaultRootSceneNodeName(proto_scene_tree.default_root_name());

    for (const auto& node_matrix : proto_scene_tree.node_matrices())
    {
        if (!ParseNodeMatrix(level, node_matrix))
        {
            return false;
        }
    }

    for (const auto& node_mesh : proto_scene_tree.node_meshes())
    {
        if (!ParseNodeMesh(level, node_mesh, options))
        {
            return false;
        }
    }

    for (const auto& node_camera : proto_scene_tree.node_cameras())
    {
        if (!ParseNodeCamera(level, node_camera))
        {
            return false;
        }
    }

    for (const auto& node_light : proto_scene_tree.node_lights())
    {
        if (!ParseNodeLight(level, node_light))
        {
            return false;
        }
    }

    FinalizeRaytracingSceneMaterials(
        level, options.prefer_hardware_raytracing);

    return true;
}

} // namespace frame::vulkan::json
