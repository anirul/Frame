#include "renderer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <format>
#include <glad/glad.h>
#include <limits>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>
#include <stdexcept>

#include "frame/bvh.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/program_key.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/cubemap_views.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/material.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/uniform_collection_wrapper.h"

namespace frame::opengl
{

namespace
{

bool IsRaytracingProgram(const ProgramInterface& program)
{
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
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
    if (!IsRaytracingProgram(program))
    {
        return false;
    }
    return material.GetPreprocessProgramId(&level) != frame::NullId;
}

bool IsRaytracingResolveMaterial(
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
    if (!IsRaytracingProgram(program))
    {
        return false;
    }
    return material.GetPreprocessProgramId(&level) == frame::NullId;
}

std::unique_ptr<Program> CreateGpuSkinningProgram()
{
    std::ifstream compute_ifs{
        frame::file::FindFile(
            std::filesystem::path("asset/shader/opengl/skinning.comp"))};
    if (!compute_ifs)
    {
        throw std::runtime_error(
            "Could not open OpenGL GPU skinning compute shader.");
    }

    std::string compute_source(
        std::istreambuf_iterator<char>(compute_ifs), {});
    auto program = std::make_unique<Program>("GpuSkinningProgram");
    Shader compute_shader(ShaderEnum::COMPUTE_SHADER);
    if (!compute_shader.LoadFromSource(compute_source))
    {
        throw std::runtime_error(compute_shader.GetErrorMessage());
    }
    program->AddShader(compute_shader);
    program->LinkShader();
    return program;
}

std::unique_ptr<Program> CreateGpuRaytraceTriangleCopyProgram()
{
    std::ifstream compute_ifs{
        frame::file::FindFile(
            std::filesystem::path(
                "asset/shader/opengl/raytrace_triangle_copy.comp"))};
    if (!compute_ifs)
    {
        throw std::runtime_error(
            "Could not open OpenGL raytrace triangle copy compute shader.");
    }

    std::string compute_source(
        std::istreambuf_iterator<char>(compute_ifs), {});
    auto program = std::make_unique<Program>("GpuRaytraceTriangleCopyProgram");
    Shader compute_shader(ShaderEnum::COMPUTE_SHADER);
    if (!compute_shader.LoadFromSource(compute_source))
    {
        throw std::runtime_error(compute_shader.GetErrorMessage());
    }
    program->AddShader(compute_shader);
    program->LinkShader();
    return program;
}

std::unique_ptr<Program> CreateGpuRaytraceBvhRefitProgram()
{
    std::ifstream compute_ifs{
        frame::file::FindFile(
            std::filesystem::path("asset/shader/opengl/raytrace_bvh_refit.comp"))};
    if (!compute_ifs)
    {
        throw std::runtime_error(
            "Could not open OpenGL raytrace BVH refit compute shader.");
    }

    std::string compute_source(
        std::istreambuf_iterator<char>(compute_ifs), {});
    auto program = std::make_unique<Program>("GpuRaytraceBvhRefitProgram");
    Shader compute_shader(ShaderEnum::COMPUTE_SHADER);
    if (!compute_shader.LoadFromSource(compute_source))
    {
        throw std::runtime_error(compute_shader.GetErrorMessage());
    }
    program->AddShader(compute_shader);
    program->LinkShader();
    return program;
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

bool RequiresWorldSpaceRaytraceBuffers(const SkinnedMesh& skinned_mesh)
{
    if (!skinned_mesh.HasActiveSkinning() &&
        !skinned_mesh.HasActiveRaytraceTriangleCallback() &&
        !skinned_mesh.HasActiveRaytraceBvhCallback())
    {
        return false;
    }
    return !skinned_mesh.SupportsGpuRaytraceSkinning();
}

bool RaytraceSceneRequiresWorldSpaceBuffers(frame::LevelInterface& level)
{
    for (const auto& [node_id, material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)material_id;
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        auto* skinned_mesh =
            dynamic_cast<SkinnedMesh*>(&level.GetMeshFromId(mesh_id));
        if (!skinned_mesh)
        {
            continue;
        }
        if (RequiresWorldSpaceRaytraceBuffers(*skinned_mesh))
        {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<EntityId, std::string>> GetActiveTextureBindings(
    const MaterialInterface& material,
    const ProgramInterface& program)
{
    std::vector<std::pair<EntityId, std::string>> bindings = {};
    for (const auto texture_id : material.GetTextureIds())
    {
        const auto inner_name = material.GetInnerName(texture_id);
        if (!program.HasUniform(inner_name))
        {
            continue;
        }
        bindings.emplace_back(texture_id, inner_name);
    }
    std::sort(
        bindings.begin(),
        bindings.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.second < rhs.second;
        });
    return bindings;
}

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
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 255.0f;
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
            alpha_index >= 0 ? read_channel(static_cast<std::size_t>(alpha_index))
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

std::array<float, 4> ResolveRaytracingSourceMaterialColor(
    frame::LevelInterface& level,
    frame::EntityId material_id)
{
    constexpr std::array<float, 4> kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!material_id)
    {
        return kWhite;
    }

    const auto& material = level.GetMaterialFromId(material_id);
    auto color_texture_id = FindTextureIdByInnerName(material, "albedo_texture");
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
    frame::LevelInterface& level,
    bool transmissive)
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

template <typename T>
void HashCombine(std::size_t& seed, const T& value)
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

glm::mat4 InverseOrIdentity(const glm::mat4& matrix)
{
    const float determinant = glm::determinant(glm::mat3(matrix));
    if (std::abs(determinant) <= 1.0e-8f)
    {
        return glm::mat4(1.0f);
    }
    return glm::inverse(matrix);
}

std::size_t GetBufferSizeBytes(const Buffer& buffer)
{
    const auto& raw = buffer.GetRawData();
    return raw.empty() ? buffer.GetSize() : raw.size();
}

constexpr std::size_t GetRaytraceTriangleStrideBytes()
{
    return sizeof(float) * 12u;
}

struct alignas(16) RaytraceInstanceStorageData
{
    glm::mat4 object_to_world = glm::mat4(1.0f);
    glm::mat4 world_to_object = glm::mat4(1.0f);
    glm::uvec4 metadata = glm::uvec4(0u);
};

struct RaytracingSourceGeometryData
{
    EntityId source_node_id = NullId;
    EntityId source_material_id = NullId;
    EntityId triangle_buffer_id = NullId;
    EntityId bvh_buffer_id = NullId;
    std::uint32_t material_id = 0u;
    std::uint32_t triangle_offset = 0u;
    std::uint32_t triangle_count = 0u;
    std::uint32_t bvh_node_offset = 0u;
    std::uint32_t bvh_node_count = 0u;
    std::array<float, 4> color_multiplier = {1.0f, 1.0f, 1.0f, 1.0f};
};

std::uint64_t BuildSourceGeometryKey(
    EntityId source_node_id, EntityId source_material_id)
{
    return (static_cast<std::uint64_t>(source_node_id) << 32u) |
           static_cast<std::uint32_t>(source_material_id);
}

std::vector<RaytracingSourceGeometryData> BuildRaytracingSourceGeometryData(
    frame::LevelInterface& level)
{
    std::vector<RaytracingSourceGeometryData> geometries = {};
    std::uint32_t next_transmissive_triangle_offset = 0u;
    std::uint32_t next_opaque_triangle_offset = 0u;
    std::uint32_t next_transmissive_bvh_offset = 0u;
    std::uint32_t next_opaque_bvh_offset = 0u;
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
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

        auto* triangle_buffer = dynamic_cast<Buffer*>(
            &level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        const bool transmissive =
            IsTransmissiveMaterial(level, source_material_id);
        const auto reference_color = ResolveRaytracingReferenceColor(
            level,
            transmissive);
        const auto source_color = ResolveRaytracingSourceMaterialColor(
            level,
            source_material_id);
        const auto color_multiplier = ResolveRaytracingColorMultiplier(
            source_color,
            reference_color);

        const auto triangle_byte_size = GetBufferSizeBytes(*triangle_buffer);
        const auto triangle_count = static_cast<std::uint32_t>(
            triangle_byte_size / (GetRaytraceTriangleStrideBytes() * 3u));

        std::uint32_t bvh_node_count = 0u;
        const auto bvh_buffer_id = mesh.GetBvhBufferId();
        if (bvh_buffer_id)
        {
            auto* bvh_buffer = dynamic_cast<Buffer*>(
                &level.GetBufferFromId(bvh_buffer_id));
            if (bvh_buffer)
            {
                bvh_node_count = static_cast<std::uint32_t>(
                    GetBufferSizeBytes(*bvh_buffer) / sizeof(frame::BVHNode));
            }
        }

        RaytracingSourceGeometryData geometry = {};
        geometry.source_node_id = source_node_id;
        geometry.source_material_id = source_material_id;
        geometry.triangle_buffer_id = triangle_buffer_id;
        geometry.bvh_buffer_id = bvh_buffer_id;
        geometry.material_id = transmissive ? 0u : 1u;
        geometry.triangle_offset = transmissive
            ? next_transmissive_triangle_offset
            : next_opaque_triangle_offset;
        geometry.triangle_count = triangle_count;
        geometry.bvh_node_offset = transmissive
            ? next_transmissive_bvh_offset
            : next_opaque_bvh_offset;
        geometry.bvh_node_count = bvh_node_count;
        geometry.color_multiplier = color_multiplier;
        geometries.push_back(geometry);

        if (transmissive)
        {
            next_transmissive_triangle_offset += triangle_count;
            next_transmissive_bvh_offset += bvh_node_count;
        }
        else
        {
            next_opaque_triangle_offset += triangle_count;
            next_opaque_bvh_offset += bvh_node_count;
        }
    }
    return geometries;
}

std::size_t BuildSourceInstanceLayoutHash(
    const std::vector<RaytracingSourceGeometryData>& source_geometries)
{
    std::size_t seed = 0;
    HashCombine(seed, source_geometries.size());
    for (const auto& geometry : source_geometries)
    {
        HashCombine(seed, static_cast<std::uint64_t>(geometry.source_node_id));
        HashCombine(
            seed,
            static_cast<std::uint64_t>(geometry.source_material_id));
        HashCombine(seed, static_cast<std::uint64_t>(geometry.triangle_buffer_id));
        HashCombine(seed, static_cast<std::uint64_t>(geometry.bvh_buffer_id));
        HashCombine(seed, geometry.material_id);
        HashCombine(seed, geometry.triangle_offset);
        HashCombine(seed, geometry.triangle_count);
        HashCombine(seed, geometry.bvh_node_offset);
        HashCombine(seed, geometry.bvh_node_count);
    }
    return seed;
}

std::size_t BuildSourceGeometryContentStateHash(
    frame::LevelInterface& level,
    const RaytracingSourceGeometryData& geometry)
{
    std::size_t seed = 0;
    HashCombine(seed, static_cast<std::uint64_t>(geometry.triangle_buffer_id));
    auto* triangle_buffer = dynamic_cast<Buffer*>(
        &level.GetBufferFromId(geometry.triangle_buffer_id));
    if (triangle_buffer)
    {
        HashCombine(seed, triangle_buffer->GetGeneration());
        HashCombine(seed, GetBufferSizeBytes(*triangle_buffer));
    }
    HashCombine(seed, static_cast<std::uint64_t>(geometry.bvh_buffer_id));
    HashCombine(seed, geometry.triangle_offset);
    HashCombine(seed, geometry.triangle_count);
    HashCombine(seed, geometry.bvh_node_offset);
    HashCombine(seed, geometry.bvh_node_count);
    HashColor(seed, geometry.color_multiplier);
    return seed;
}

std::vector<std::uint8_t> BuildRaytraceInstanceBytes(
    frame::LevelInterface& level,
    const std::vector<RaytracingSourceGeometryData>& source_geometries,
    double time_seconds)
{
    std::vector<RaytraceInstanceStorageData> instances = {};
    instances.reserve(source_geometries.size());
    for (const auto& geometry : source_geometries)
    {
        RaytraceInstanceStorageData instance = {};
        auto* node = dynamic_cast<NodeMesh*>(
            &level.GetSceneNodeFromId(geometry.source_node_id));
        if (node)
        {
            instance.object_to_world = node->GetLocalModel(time_seconds);
            instance.world_to_object =
                InverseOrIdentity(instance.object_to_world);
        }
        instance.metadata.x = geometry.triangle_offset;
        instance.metadata.y = geometry.material_id;
        instance.metadata.z = geometry.bvh_node_count > 0u
            ? geometry.bvh_node_offset
            : std::numeric_limits<std::uint32_t>::max();
        instance.metadata.w = geometry.triangle_count;
        instances.push_back(instance);
    }

    std::vector<std::uint8_t> bytes(
        instances.size() * sizeof(RaytraceInstanceStorageData));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), instances.data(), bytes.size());
    }
    return bytes;
}

std::vector<std::uint8_t> BuildSourceInstanceAggregateBvhBytes(
    frame::LevelInterface& level,
    const std::vector<RaytracingSourceGeometryData>& source_geometries,
    std::uint32_t material_id)
{
    std::vector<frame::BVHNode> aggregate_nodes = {};
    for (const auto& geometry : source_geometries)
    {
        if (geometry.material_id != material_id || !geometry.bvh_buffer_id ||
            geometry.bvh_node_count == 0u)
        {
            continue;
        }

        auto* bvh_buffer = dynamic_cast<Buffer*>(
            &level.GetBufferFromId(geometry.bvh_buffer_id));
        if (!bvh_buffer)
        {
            continue;
        }

        const auto& raw = bvh_buffer->GetRawData();
        if (raw.empty() || raw.size() % sizeof(frame::BVHNode) != 0)
        {
            continue;
        }

        const auto* nodes =
            reinterpret_cast<const frame::BVHNode*>(raw.data());
        const auto node_count = raw.size() / sizeof(frame::BVHNode);
        for (std::size_t i = 0; i < node_count; ++i)
        {
            auto node = nodes[i];
            if (node.left >= 0)
            {
                node.left += static_cast<int>(geometry.bvh_node_offset);
            }
            if (node.right >= 0)
            {
                node.right += static_cast<int>(geometry.bvh_node_offset);
            }
            if (node.triangle_count > 0 && node.first_triangle >= 0)
            {
                node.first_triangle +=
                    static_cast<int>(geometry.triangle_offset);
            }
            aggregate_nodes.push_back(node);
        }
    }

    std::vector<std::uint8_t> bytes(
        aggregate_nodes.size() * sizeof(frame::BVHNode));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), aggregate_nodes.data(), bytes.size());
    }
    return bytes;
}

std::size_t BuildRaytracingSourceStateHash(
    frame::LevelInterface& level,
    double time_seconds)
{
    std::size_t state_hash = 0;
    const auto source_mesh_materials = GetRaytracingSourceMeshMaterials(level);
    HashCombine(state_hash, source_mesh_materials.size());
    for (const auto& [source_node_id, source_material_id] : source_mesh_materials)
    {
        HashCombine(state_hash, static_cast<std::uint64_t>(source_node_id));
        HashCombine(state_hash, static_cast<std::uint64_t>(source_material_id));

        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            continue;
        }

        HashMatrix(state_hash, node->GetLocalModel(time_seconds));
        HashCombine(
            state_hash,
            IsTransmissiveMaterial(level, source_material_id));
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
        HashCombine(
            state_hash,
            static_cast<std::uint64_t>(triangle_buffer_id));
        if (!triangle_buffer_id)
        {
            continue;
        }

        auto* triangle_buffer = dynamic_cast<Buffer*>(
            &level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        HashCombine(state_hash, triangle_buffer->GetGeneration());
        HashCombine(state_hash, GetBufferSizeBytes(*triangle_buffer));
    }
    return state_hash;
}

constexpr std::size_t kRaytraceFloatsPerVertex = 12;
constexpr std::size_t kRaytraceTriangleVertexStrideBytes =
    sizeof(float) * kRaytraceFloatsPerVertex;
constexpr std::uint32_t kGpuSkinningWorkgroupSize = 64u;

struct RaytraceVertex
{
    float px;
    float py;
    float pz;
    float pad0;
    float nx;
    float ny;
    float nz;
    float pad1;
    float u;
    float v;
    float pad2;
    float pad3;
};

std::vector<std::uint8_t> ApplyTriangleColorMultiplier(
    const std::vector<std::uint8_t>& raw,
    const std::array<float, 4>& color)
{
    if (raw.empty())
    {
        return raw;
    }
    if (raw.size() % sizeof(RaytraceVertex) != 0)
    {
        throw std::runtime_error(
            "Animated OpenGL raytrace triangle buffer size is not aligned to the expected vertex stride.");
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
    const std::vector<std::uint8_t>& raw,
    const glm::mat4& model)
{
    if (raw.empty() || model == glm::mat4(1.0f))
    {
        return raw;
    }
    if (raw.size() % sizeof(RaytraceVertex) != 0)
    {
        throw std::runtime_error(
            "Animated OpenGL raytrace triangle buffer size is not aligned to the expected vertex stride.");
    }

    std::vector<std::uint8_t> transformed = raw;
    auto* vertices = reinterpret_cast<RaytraceVertex*>(transformed.data());
    const std::size_t vertex_count = transformed.size() / sizeof(RaytraceVertex);

    glm::mat3 normal_matrix = glm::mat3(1.0f);
    const float det = glm::determinant(glm::mat3(model));
    if (std::abs(det) > 1.0e-8f)
    {
        normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
    }

    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        const glm::vec3 position = glm::vec3(
            model * glm::vec4(vertices[i].px, vertices[i].py, vertices[i].pz, 1.0f));
        vertices[i].px = position.x;
        vertices[i].py = position.y;
        vertices[i].pz = position.z;

        glm::vec3 normal = normal_matrix *
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

std::vector<std::uint8_t> BuildAggregateTriangleBytes(
    frame::LevelInterface& level,
    bool transmissive,
    double time_seconds)
{
    std::vector<std::uint8_t> aggregate_triangle_bytes = {};
    const auto reference_color =
        ResolveRaytracingReferenceColor(level, transmissive);
    for (const auto& [source_node_id, source_material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        if (IsTransmissiveMaterial(level, source_material_id) != transmissive)
        {
            continue;
        }

        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
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

        auto* triangle_buffer = dynamic_cast<opengl::Buffer*>(
            &level.GetBufferFromId(triangle_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        const auto source_color =
            ResolveRaytracingSourceMaterialColor(level, source_material_id);
        const auto color_multiplier = ResolveRaytracingColorMultiplier(
            source_color,
            reference_color);
        const auto transformed = TransformTriangleBytes(
            triangle_buffer->GetRawData(),
            node->GetLocalModel(time_seconds));
        const auto tinted =
            ApplyTriangleColorMultiplier(transformed, color_multiplier);
        aggregate_triangle_bytes.insert(
            aggregate_triangle_bytes.end(),
            tinted.begin(),
            tinted.end());
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
            "Animated OpenGL raytrace triangle buffer size is not aligned to the expected vertex stride.");
    }

    const auto* triangle_floats =
        reinterpret_cast<const float*>(triangle_bytes.data());
    const std::size_t vertex_count =
        triangle_bytes.size() / kRaytraceTriangleVertexStrideBytes;
    std::vector<float> points = {};
    points.reserve(vertex_count * 3);
    for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
    {
        const std::size_t base = vertex_index * kRaytraceFloatsPerVertex;
        points.push_back(triangle_floats[base + 0]);
        points.push_back(triangle_floats[base + 1]);
        points.push_back(triangle_floats[base + 2]);
    }

    std::vector<std::uint32_t> indices(vertex_count);
    std::iota(indices.begin(), indices.end(), 0u);
    const auto bvh_nodes = frame::BuildBVH(points, indices);

    std::vector<std::uint8_t> bytes(
        bvh_nodes.size() * sizeof(frame::BVHNode));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), bvh_nodes.data(), bytes.size());
    }
    return bytes;
}

} // namespace

Renderer::Renderer(LevelInterface& level, glm::uvec4 viewport)
    : level_(level), viewport_(viewport)
{
    frame_buffer_ = std::make_unique<FrameBuffer>();
    render_buffer_ = std::make_unique<RenderBuffer>();
    // TODO(anirul): Check viewport!!!
    render_buffer_->CreateStorage(
        {viewport_.z - viewport_.x, viewport_.w - viewport_.y});
    frame_buffer_->AttachRender(*render_buffer_);
    proto::Program proto_program;
    proto_program.set_name("display");
    proto_program.set_pipeline_name("display");
    auto program = file::LoadProgram(
        proto_program,
        "asset/shader/opengl/display.vert",
        "asset/shader/opengl/display.frag");
    if (!program)
        throw std::runtime_error("No program!");
    auto material = std::make_unique<Material>();
    program->SetName("DisplayProgram");
    program->SetSerializeEnable(false);
    auto maybe_display_program_id = level_.AddProgram(std::move(program));
    if (!maybe_display_program_id)
        throw std::runtime_error("No display program id.");
    display_program_id_ = maybe_display_program_id;
    material->SetName("DisplayMaterial");
    material->SetSerializeEnable(false);
    auto maybe_display_material_id = level_.AddMaterial(std::move(material));
    if (!maybe_display_material_id)
        throw std::runtime_error("No display material id.");
    display_material_id_ = maybe_display_material_id;
    auto maybe_out_texture_id = level_.GetDefaultOutputTextureId();
    if (!maybe_out_texture_id)
    {
        throw std::runtime_error("No output texture id.");
    }
    auto out_texture_id = maybe_out_texture_id;
    auto& out_texture = level_.GetTextureFromId(out_texture_id);
    // Get material from level as material was moved away.
    level_.GetMaterialFromId(display_material_id_)
        .SetProgramId(display_program_id_);
    if (!level_.GetMaterialFromId(display_material_id_)
             .AddTextureId(out_texture_id, "Display"))
    {
        throw std::runtime_error("Couldn't add texture to material.");
    }
    gpu_skinning_bone_matrix_buffer_ = std::make_unique<Buffer>(
        BufferTypeEnum::SHADER_STORAGE_BUFFER,
        BufferUsageEnum::DYNAMIC_DRAW);
    gpu_skinning_bone_matrix_buffer_->SetName(
        "Renderer.GpuSkinning.BoneMatrices");
    gpu_skinning_empty_buffer_ = std::make_unique<Buffer>(
        BufferTypeEnum::SHADER_STORAGE_BUFFER,
        BufferUsageEnum::STATIC_DRAW);
    gpu_skinning_empty_buffer_->SetName("Renderer.GpuSkinning.Empty");
    gpu_skinning_empty_buffer_->Copy(std::vector<float>{0.0f});
}

void Renderer::EnsureGpuSkinningProgram()
{
    if (gpu_skinning_program_attempted_)
    {
        return;
    }
    gpu_skinning_program_attempted_ = true;
    if (glDispatchCompute == nullptr)
    {
        logger_->warn(
            "OpenGL compute dispatch is unavailable; GPU skinning fallback "
            "stays on CPU.");
        return;
    }
    try
    {
        gpu_skinning_program_ = CreateGpuSkinningProgram();
    }
    catch (const std::exception& error)
    {
        logger_->warn(
            "Failed to create OpenGL GPU skinning program: {}",
            error.what());
        gpu_skinning_program_.reset();
    }
}

void Renderer::EnsureGpuRaytraceTriangleCopyProgram()
{
    if (gpu_raytrace_triangle_copy_program_attempted_)
    {
        return;
    }
    gpu_raytrace_triangle_copy_program_attempted_ = true;
    if (glDispatchCompute == nullptr)
    {
        logger_->warn(
            "OpenGL compute dispatch is unavailable; GPU raytrace triangle "
            "copy is disabled.");
        return;
    }
    try
    {
        gpu_raytrace_triangle_copy_program_ =
            CreateGpuRaytraceTriangleCopyProgram();
    }
    catch (const std::exception& error)
    {
        logger_->warn(
            "Failed to create OpenGL raytrace triangle copy program: {}",
            error.what());
        gpu_raytrace_triangle_copy_program_.reset();
    }
}

void Renderer::EnsureGpuRaytraceBvhRefitProgram()
{
    if (gpu_raytrace_bvh_refit_program_attempted_)
    {
        return;
    }
    gpu_raytrace_bvh_refit_program_attempted_ = true;
    if (glDispatchCompute == nullptr)
    {
        logger_->warn(
            "OpenGL compute dispatch is unavailable; GPU raytrace BVH refit "
            "is disabled.");
        return;
    }
    try
    {
        gpu_raytrace_bvh_refit_program_ = CreateGpuRaytraceBvhRefitProgram();
    }
    catch (const std::exception& error)
    {
        logger_->warn(
            "Failed to create OpenGL raytrace BVH refit program: {}",
            error.what());
        gpu_raytrace_bvh_refit_program_.reset();
    }
}

bool Renderer::UpdateRaytraceBuffersOnGpuIfPossible(SkinnedMesh& skinned_mesh)
{
    if (!skinned_mesh.HasActiveRaytraceTriangleCallback() ||
        !skinned_mesh.SupportsGpuRaytraceSkinning())
    {
        return false;
    }

    const EntityId point_buffer_id = skinned_mesh.GetPointBufferId();
    const EntityId index_buffer_id = skinned_mesh.GetIndexBufferId();
    const EntityId triangle_buffer_id = skinned_mesh.GetTriangleBufferId();
    const EntityId bone_index_buffer_id = skinned_mesh.GetBoneIndexBufferId();
    const EntityId bone_weight_buffer_id = skinned_mesh.GetBoneWeightBufferId();
    if (!point_buffer_id || !index_buffer_id || !triangle_buffer_id ||
        !bone_index_buffer_id || !bone_weight_buffer_id)
    {
        return false;
    }

    EnsureGpuSkinningProgram();
    if (!gpu_skinning_program_ || !gpu_skinning_bone_matrix_buffer_ ||
        !gpu_skinning_empty_buffer_)
    {
        return false;
    }

    const double skinning_time = skinned_mesh.GetSkinningTime(delta_time_);
    if (!skinned_mesh.ShouldUpdateRaytraceBuffers(skinning_time))
    {
        return true;
    }
    auto bone_matrices = skinned_mesh.EvaluateSkinning(skinning_time);
    if (bone_matrices.empty())
    {
        bone_matrices.push_back(glm::mat4(1.0f));
    }
    gpu_skinning_bone_matrix_buffer_->Copy(
        bone_matrices.size() * sizeof(glm::mat4),
        bone_matrices.data());

    auto& point_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(point_buffer_id));
    auto* normal_buffer = skinned_mesh.GetNormalBufferId()
        ? dynamic_cast<Buffer*>(
              &level_.GetBufferFromId(skinned_mesh.GetNormalBufferId()))
        : nullptr;
    auto* texture_buffer = skinned_mesh.GetTextureBufferId()
        ? dynamic_cast<Buffer*>(
              &level_.GetBufferFromId(skinned_mesh.GetTextureBufferId()))
        : nullptr;
    auto& index_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(index_buffer_id));
    auto& bone_index_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(bone_index_buffer_id));
    auto& bone_weight_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(bone_weight_buffer_id));
    auto& triangle_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(triangle_buffer_id));

    const std::size_t index_count =
        skinned_mesh.GetIndexSize() / sizeof(std::uint32_t);
    if (index_count == 0)
    {
        skinned_mesh.MarkRaytraceBuffersUpdated(skinning_time);
        return true;
    }

    gpu_skinning_program_->Use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, point_buffer.GetId());
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        1,
        normal_buffer
            ? normal_buffer->GetId()
            : gpu_skinning_empty_buffer_->GetId());
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        2,
        texture_buffer
            ? texture_buffer->GetId()
            : gpu_skinning_empty_buffer_->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, index_buffer.GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bone_index_buffer.GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bone_weight_buffer.GetId());
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        6,
        gpu_skinning_bone_matrix_buffer_->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, triangle_buffer.GetId());
    glDispatchCompute(
        static_cast<GLuint>(
            (index_count + kGpuSkinningWorkgroupSize - 1u) /
            kGpuSkinningWorkgroupSize),
        1,
        1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    for (GLuint binding = 0; binding < 8u; ++binding)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
    }
    gpu_skinning_program_->UnUse();
    triangle_buffer.MarkGpuModified();
    skinned_mesh.MarkRaytraceBuffersUpdated(skinning_time);
    return true;
}

bool Renderer::UpdateRaytraceBuffersIfNeeded(SkinnedMesh& skinned_mesh)
{
    const double skinning_time = skinned_mesh.GetSkinningTime(delta_time_);
    if (!skinned_mesh.ShouldUpdateRaytraceBuffers(skinning_time))
    {
        return false;
    }
    if (UpdateRaytraceBuffersOnGpuIfPossible(skinned_mesh))
    {
        return true;
    }

    bool updated = false;

    if (skinned_mesh.HasActiveRaytraceTriangleCallback())
    {
        const EntityId triangle_buffer_id = skinned_mesh.GetTriangleBufferId();
        if (triangle_buffer_id)
        {
            auto triangles =
                skinned_mesh.EvaluateRaytraceTriangles(skinning_time);
            if (!triangles.empty())
            {
                auto& triangle_buffer = dynamic_cast<Buffer&>(
                    level_.GetBufferFromId(triangle_buffer_id));
                triangle_buffer.Copy(triangles);
                updated = true;
            }
        }
    }

    if (skinned_mesh.HasActiveRaytraceBvhCallback())
    {
        const EntityId bvh_buffer_id = skinned_mesh.GetBvhBufferId();
        if (bvh_buffer_id)
        {
            auto bvh_nodes = skinned_mesh.EvaluateRaytraceBvh(skinning_time);
            if (!bvh_nodes.empty())
            {
                auto& bvh_buffer =
                    dynamic_cast<Buffer&>(level_.GetBufferFromId(bvh_buffer_id));
                bvh_buffer.Copy(
                    bvh_nodes.size() * sizeof(frame::BVHNode),
                    bvh_nodes.data());
                updated = true;
            }
        }
    }
    if (updated)
    {
        skinned_mesh.MarkRaytraceBuffersUpdated(skinning_time);
    }
    return updated;
}

void Renderer::UpdateSourceInstanceRaytraceSceneBuffers()
{
    const std::size_t scene_state_hash =
        BuildRaytracingSourceStateHash(level_, delta_time_);
    if (has_raytrace_scene_state_hash_ &&
        last_raytrace_scene_state_hash_ == scene_state_hash)
    {
        return;
    }

    const auto source_geometries = BuildRaytracingSourceGeometryData(level_);
    const auto instance_bytes =
        BuildRaytraceInstanceBytes(level_, source_geometries, delta_time_);
    const auto layout_hash = BuildSourceInstanceLayoutHash(source_geometries);
    const bool layout_changed = !has_source_instance_layout_hash_ ||
        last_source_instance_layout_hash_ != layout_hash;

    std::unordered_map<std::uint64_t, std::size_t> next_geometry_state_hashes = {};
    next_geometry_state_hashes.reserve(source_geometries.size());
    std::vector<const RaytracingSourceGeometryData*> updated_geometries = {};
    updated_geometries.reserve(source_geometries.size());
    for (const auto& geometry : source_geometries)
    {
        const auto key = BuildSourceGeometryKey(
            geometry.source_node_id,
            geometry.source_material_id);
        const auto state_hash =
            BuildSourceGeometryContentStateHash(level_, geometry);
        next_geometry_state_hashes[key] = state_hash;
        const auto previous =
            raytrace_source_geometry_state_hashes_.find(key);
        if (layout_changed || previous == raytrace_source_geometry_state_hashes_.end() ||
            previous->second != state_hash)
        {
            updated_geometries.push_back(&geometry);
        }
    }

    EnsureGpuRaytraceTriangleCopyProgram();
    EnsureGpuRaytraceBvhRefitProgram();

    const auto find_named_buffer =
        [&](MaterialInterface& material, const char* inner_name) -> Buffer* {
            for (const auto& buffer_name : material.GetBufferNames())
            {
                if (material.GetInnerBufferName(buffer_name) != inner_name)
                {
                    continue;
                }
                const auto buffer_id = level_.GetIdFromName(buffer_name);
                if (!buffer_id)
                {
                    return nullptr;
                }
                return dynamic_cast<Buffer*>(&level_.GetBufferFromId(buffer_id));
            }
            return nullptr;
        };

    const auto triangle_bytes_for_material =
        [&](std::uint32_t material_id) -> std::size_t {
            std::size_t triangle_count = 0;
            for (const auto& geometry : source_geometries)
            {
                if (geometry.material_id != material_id)
                {
                    continue;
                }
                triangle_count += geometry.triangle_count;
            }
            return triangle_count * GetRaytraceTriangleStrideBytes() * 3u;
        };

    const auto copy_triangle_range =
        [&](const RaytracingSourceGeometryData& geometry,
            Buffer& source_triangle_buffer,
            Buffer& aggregate_triangle_buffer) {
            if (!gpu_raytrace_triangle_copy_program_ ||
                geometry.triangle_count == 0u)
            {
                return;
            }

            UniformCollectionWrapper uniforms = {};
            uniforms.AddUniform(std::make_unique<Uniform>(
                "source_vertex_count",
                static_cast<int>(geometry.triangle_count * 3u)));
            uniforms.AddUniform(std::make_unique<Uniform>(
                "destination_vertex_offset",
                static_cast<int>(geometry.triangle_offset * 3u)));
            uniforms.AddUniform(std::make_unique<Uniform>(
                "color_multiplier",
                glm::vec4(
                    geometry.color_multiplier[0],
                    geometry.color_multiplier[1],
                    geometry.color_multiplier[2],
                    geometry.color_multiplier[3])));
            gpu_raytrace_triangle_copy_program_->Use(uniforms, nullptr);
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                0,
                source_triangle_buffer.GetId());
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                1,
                aggregate_triangle_buffer.GetId());
            glDispatchCompute(
                static_cast<GLuint>(
                    (geometry.triangle_count * 3u + kGpuSkinningWorkgroupSize - 1u) /
                    kGpuSkinningWorkgroupSize),
                1,
                1);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
            gpu_raytrace_triangle_copy_program_->UnUse();
        };

    const auto refit_bvh_range =
        [&](const RaytracingSourceGeometryData& geometry,
            Buffer& aggregate_triangle_buffer,
            Buffer& aggregate_bvh_buffer) {
            if (!gpu_raytrace_bvh_refit_program_ || geometry.bvh_node_count == 0u)
            {
                return;
            }

            UniformCollectionWrapper uniforms = {};
            uniforms.AddUniform(std::make_unique<Uniform>(
                "node_offset",
                static_cast<int>(geometry.bvh_node_offset)));
            uniforms.AddUniform(std::make_unique<Uniform>(
                "node_count",
                static_cast<int>(geometry.bvh_node_count)));
            gpu_raytrace_bvh_refit_program_->Use(uniforms, nullptr);
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                0,
                aggregate_triangle_buffer.GetId());
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                1,
                aggregate_bvh_buffer.GetId());
            glDispatchCompute(1, 1, 1);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
            gpu_raytrace_bvh_refit_program_->UnUse();
        };

    const auto aggregate_transmissive_bvh =
        layout_changed
            ? BuildSourceInstanceAggregateBvhBytes(level_, source_geometries, 0u)
            : std::vector<std::uint8_t>{};
    const auto aggregate_opaque_bvh =
        layout_changed
            ? BuildSourceInstanceAggregateBvhBytes(level_, source_geometries, 1u)
            : std::vector<std::uint8_t>{};

    for (const auto& [node_id, material_id] :
         level_.GetMeshMaterialIds(proto::NodeMesh::SCENE_RENDER_TIME))
    {
        (void)node_id;
        if (!material_id || !IsRaytracingResolveMaterial(level_, material_id))
        {
            continue;
        }

        auto& material = level_.GetMaterialFromId(material_id);
        auto* transmissive_triangle_buffer =
            find_named_buffer(material, "TriangleBufferTransmissive");
        auto* transmissive_bvh_buffer =
            find_named_buffer(material, "BvhBufferTransmissive");
        auto* opaque_triangle_buffer =
            find_named_buffer(material, "TriangleBufferOpaque");
        auto* opaque_bvh_buffer = find_named_buffer(material, "BvhBufferOpaque");
        auto* instance_buffer = find_named_buffer(material, "RaytraceInstanceBuffer");

        if (layout_changed)
        {
            if (transmissive_triangle_buffer)
            {
                transmissive_triangle_buffer->Copy(
                    std::vector<std::uint8_t>(
                        triangle_bytes_for_material(0u),
                        0u));
            }
            if (opaque_triangle_buffer)
            {
                opaque_triangle_buffer->Copy(
                    std::vector<std::uint8_t>(
                        triangle_bytes_for_material(1u),
                        0u));
            }
            if (transmissive_bvh_buffer)
            {
                transmissive_bvh_buffer->Copy(aggregate_transmissive_bvh);
            }
            if (opaque_bvh_buffer)
            {
                opaque_bvh_buffer->Copy(aggregate_opaque_bvh);
            }
        }

        if (instance_buffer)
        {
            instance_buffer->Copy(instance_bytes);
        }

        if (updated_geometries.empty())
        {
            continue;
        }

        for (const auto* geometry : updated_geometries)
        {
            if (!geometry || geometry->triangle_count == 0u)
            {
                continue;
            }

            auto* source_triangle_buffer = dynamic_cast<Buffer*>(
                &level_.GetBufferFromId(geometry->triangle_buffer_id));
            auto* aggregate_triangle_buffer = geometry->material_id == 0u
                ? transmissive_triangle_buffer
                : opaque_triangle_buffer;
            if (!source_triangle_buffer || !aggregate_triangle_buffer)
            {
                continue;
            }
            copy_triangle_range(
                *geometry,
                *source_triangle_buffer,
                *aggregate_triangle_buffer);
            aggregate_triangle_buffer->MarkGpuModified();
        }
        if (!updated_geometries.empty())
        {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        for (const auto* geometry : updated_geometries)
        {
            if (!geometry || geometry->bvh_node_count == 0u)
            {
                continue;
            }

            auto* aggregate_triangle_buffer = geometry->material_id == 0u
                ? transmissive_triangle_buffer
                : opaque_triangle_buffer;
            auto* aggregate_bvh_buffer = geometry->material_id == 0u
                ? transmissive_bvh_buffer
                : opaque_bvh_buffer;
            if (!aggregate_triangle_buffer || !aggregate_bvh_buffer)
            {
                continue;
            }
            refit_bvh_range(
                *geometry,
                *aggregate_triangle_buffer,
                *aggregate_bvh_buffer);
            aggregate_bvh_buffer->MarkGpuModified();
        }
        if (!updated_geometries.empty())
        {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    raytrace_source_geometry_state_hashes_ =
        std::move(next_geometry_state_hashes);
    last_source_instance_layout_hash_ = layout_hash;
    has_source_instance_layout_hash_ = true;
    last_raytrace_scene_state_hash_ = scene_state_hash;
    has_raytrace_scene_state_hash_ = true;
}

void Renderer::UpdateAggregateRaytraceSceneBuffers()
{
    const std::size_t scene_state_hash =
        BuildRaytracingSourceStateHash(level_, delta_time_);
    if (has_raytrace_scene_state_hash_ &&
        last_raytrace_scene_state_hash_ == scene_state_hash)
    {
        return;
    }

    const auto transmissive_triangles = BuildAggregateTriangleBytes(
        level_,
        true,
        delta_time_);
    const auto opaque_triangles = BuildAggregateTriangleBytes(
        level_,
        false,
        delta_time_);
    const auto transmissive_bvh = BuildAggregateBvhBytes(transmissive_triangles);
    const auto opaque_bvh = BuildAggregateBvhBytes(opaque_triangles);

    auto update_named_buffer = [&](MaterialInterface& material,
                                   const char* inner_name,
                                   const std::vector<std::uint8_t>& bytes) {
        for (const auto& buffer_name : material.GetBufferNames())
        {
            if (material.GetInnerBufferName(buffer_name) != inner_name)
            {
                continue;
            }
            const auto buffer_id = level_.GetIdFromName(buffer_name);
            if (!buffer_id)
            {
                continue;
            }
            auto* buffer = dynamic_cast<Buffer*>(&level_.GetBufferFromId(buffer_id));
            if (!buffer || buffer->GetRawData() == bytes)
            {
                return;
            }
            buffer->Copy(bytes);
            return;
        }
    };

    for (const auto& [node_id, material_id] :
         level_.GetMeshMaterialIds(proto::NodeMesh::SCENE_RENDER_TIME))
    {
        (void)node_id;
        if (!material_id ||
            !IsRaytracingResolveMaterial(level_, material_id))
        {
            continue;
        }
        auto& material = level_.GetMaterialFromId(material_id);

        update_named_buffer(
            material, "TriangleBufferTransmissive", transmissive_triangles);
        update_named_buffer(
            material, "BvhBufferTransmissive", transmissive_bvh);
        update_named_buffer(material, "TriangleBufferOpaque", opaque_triangles);
        update_named_buffer(material, "BvhBufferOpaque", opaque_bvh);
    }

    last_raytrace_scene_state_hash_ = scene_state_hash;
    has_raytrace_scene_state_hash_ = true;
}

std::optional<glm::mat4> Renderer::RenderNode(
    EntityId node_id,
    EntityId material_id,
    const glm::mat4& projection,
    const glm::mat4& view)
{
    // Bail out in case of no node.
    if (node_id == NullId)
        return std::nullopt;
    // Check current node.
    auto& node = level_.GetSceneNodeFromId(node_id);
    // Try to cast to a node Mesh.
    auto& node_mesh = dynamic_cast<NodeMesh&>(node);
    auto mesh_id = node.GetLocalMesh();
    // In case no mesh then this is a clear event.
    if (!mesh_id)
    {
        GLbitfield bit_field = 0;
        std::uint32_t clean_buffer = 0;
        for (const auto clean_elem :
             node_mesh.GetData().clean_buffer().values())
        {
            clean_buffer |= static_cast<std::uint32_t>(clean_elem);
        }
        if (clean_buffer & proto::CleanBuffer::CLEAR_COLOR)
        {
            bit_field |= GL_COLOR_BUFFER_BIT;
        }
        if (clean_buffer & proto::CleanBuffer::CLEAR_DEPTH)
        {
            bit_field |= GL_DEPTH_BUFFER_BIT;
        }
        if (bit_field)
        {
            glClear(bit_field);
        }
        return std::nullopt;
    }
    auto& mesh = level_.GetMeshFromId(mesh_id);
    // Try to find the material for the mesh.
    if (material_id == NullId)
    {
        throw std::runtime_error("No material?");
    }
    MaterialInterface& material = level_.GetMaterialFromId(material_id);
    EntityId program_id = material.GetProgramId(&level_);
    if (!program_id)
    {
        program_id = level_.GetRenderPassProgramId(
            node_mesh.GetData().render_time_enum());
        if (program_id)
        {
            material.SetProgramId(program_id);
        }
    }
    if (!program_id)
    {
        throw std::runtime_error("No program configured for material.");
    }
    auto& program = level_.GetProgramFromId(program_id);
    glm::mat4 model = node.GetLocalModel(delta_time_);
    MeshInterface* mesh_to_render = &mesh;
    const EntityId quad_id = level_.GetDefaultMeshQuadId();
    if (quad_id != NullId &&
        mesh_id != quad_id &&
        IsRaytracingProgram(program))
    {
        if (auto* gl_skinned_mesh = dynamic_cast<SkinnedMesh*>(&mesh))
        {
            UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
        }
        mesh_to_render = &level_.GetMeshFromId(quad_id);
    }
    RenderMesh(*mesh_to_render, material, projection, view, model);
    return model;
}

void Renderer::RenderMesh(
    MeshInterface& mesh,
    MaterialInterface& material,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model /* = glm::mat4(1.0f)*/)
{
    auto program_id = material.GetProgramId();
    auto& program = level_.GetProgramFromId(program_id);
    glm::mat4 model_matrix = model;
    if (IsRaytracingProgram(program) &&
        render_time_ == proto::NodeMesh::SCENE_RENDER_TIME)
    {
        if (RaytraceSceneRequiresWorldSpaceBuffers(level_))
        {
            model_matrix = glm::mat4(1.0f);
        }
        else if (!program.GetTemporarySceneRoot().empty())
        {
            auto temp_id = level_.GetIdFromName(program.GetTemporarySceneRoot());
            if (temp_id != NullId)
            {
                auto& temp_node = level_.GetSceneNodeFromId(temp_id);
                model_matrix = temp_node.GetLocalModel(delta_time_);
            }
        }
    }
    else if (!program.GetTemporarySceneRoot().empty())
    {
        auto temp_id = level_.GetIdFromName(program.GetTemporarySceneRoot());
        if (temp_id != NullId)
        {
            auto& temp_node = level_.GetSceneNodeFromId(temp_id);
            model_matrix = temp_node.GetLocalModel(delta_time_);
        }
    }

    // In case the camera doesn't exist it will create a basic one.
    UniformCollectionWrapper uniform_collection_wrapper(
        projection, view, model_matrix, delta_time_);
    const auto light_id = FindPreferredRaytraceLightId(level_);
    if (light_id != NullId)
    {
        auto& light = level_.GetLightFromId(light_id);
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>("light_dir", light.GetVector()));
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>(
                "light_type", static_cast<int>(light.GetType())));
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>(
                "light_color", light.GetColorIntensity()));
    }
    if (render_time_ == proto::NodeMesh::SCENE_RENDER_TIME)
    {
        std::unique_ptr<UniformInterface> env_map_uniform =
            std::make_unique<Uniform>("env_map_model", env_map_model_);
        uniform_collection_wrapper.AddUniform(std::move(env_map_uniform));
    }
    // Go through the callback.
    callback_(uniform_collection_wrapper, mesh, material);

    // Add node-based model matrices.
    for (const auto& name : material.GetNodeNames())
    {
        auto node_id = level_.GetIdFromName(name);
        if (node_id == NullId)
        {
            throw std::runtime_error("Could not find node: " + name);
        }
        auto& node = level_.GetSceneNodeFromId(node_id);
        glm::mat4 node_model = node.GetLocalModel(delta_time_);
        auto inner_name = material.GetInnerNodeName(name);
        std::unique_ptr<UniformInterface> node_uniform =
            std::make_unique<Uniform>(inner_name, node_model);
        uniform_collection_wrapper.AddUniform(std::move(node_uniform));
    }

    // Register shader storage buffers before using the program so they are
    // bound when Program::Use uploads them.
    int j = 0;
    for (const auto& name : material.GetBufferNames())
    {
        auto id = level_.GetIdFromName(name);
        if (id == NullId)
        {
            throw std::runtime_error("Could not find buffer: " + name);
        }
        auto inner_name = material.GetInnerBufferName(name);
        dynamic_cast<opengl::Program&>(program).AddBuffer(id, inner_name, j++);
    }

    auto& gl_mesh = dynamic_cast<Mesh&>(mesh);
    auto* gl_skinned_mesh = dynamic_cast<SkinnedMesh*>(&gl_mesh);
    if (gl_skinned_mesh)
    {
        UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
    }
    program.Use(uniform_collection_wrapper, &level_);
    int skinning_enabled = 0;
    auto& gl_program = dynamic_cast<opengl::Program&>(program);
    if (gl_skinned_mesh && gl_skinned_mesh->HasActiveSkinning())
    {
        const double skinning_time =
            gl_skinned_mesh->GetSkinningTime(delta_time_);
        auto bone_matrices = gl_skinned_mesh->EvaluateSkinning(skinning_time);
        if (!bone_matrices.empty())
        {
            constexpr std::size_t kMaxBones = 128;
            if (bone_matrices.size() > kMaxBones)
            {
                bone_matrices.resize(kMaxBones);
            }
            gl_program.UploadMatrix4ArrayUniform(
                "bone_matrices", bone_matrices);
            skinning_enabled = 1;
        }
    }
    if (program.HasUniform("skinning_enabled"))
    {
        program.AddUniform(
            std::make_unique<Uniform>("skinning_enabled", skinning_enabled));
    }

    auto texture_out_ids = program.GetOutputTextureIds();
    glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w);
    std::unique_ptr<ScopedBind> scoped_frame;
    if (!texture_out_ids.empty())
    {
        scoped_frame = std::make_unique<ScopedBind>(*frame_buffer_);
        int i = 0;
        for (const auto& texture_id : texture_out_ids)
        {
            if (level_.GetTextureFromId(texture_id).GetData().cubemap())
            {
                auto& opengl_texture =
                    dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
                // TODO(anirul): Check the mipmap level (last parameter)!
                frame_buffer_->AttachTexture(
                    opengl_texture.GetId(),
                    FrameBuffer::GetFrameColorAttachment(i),
                    FrameBuffer::GetFrameTextureType(texture_frame_),
                    0);
            }
            else
            {
                auto& opengl_texture =
                    dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
                // TODO(anirul): Check the mipmap level (last parameter)!
                frame_buffer_->AttachTexture(
                    opengl_texture.GetId(),
                    FrameBuffer::GetFrameColorAttachment(i),
                    FrameTextureType::TEXTURE_2D,
                    0);
            }
            i++;
        }
        frame_buffer_->DrawBuffers(
            static_cast<std::uint32_t>(texture_out_ids.size()));
    }
    else
    {
        scoped_frame = std::make_unique<ScopedBind>(*frame_buffer_);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    const auto active_texture_bindings =
        GetActiveTextureBindings(material, program);
    for (const auto& [id, inner_name] : active_texture_bindings)
    {
        EntityId texture_id = NullId;
        if (level_.GetEnumTypeFromId(id) == EntityTypeEnum::TEXTURE)
        {
            texture_id = id;
        }
        else
        {
            // TODO(anirul): Find a better way to find the texture
            // associated with the stream.
            texture_id = id + 1;
        }
        const auto p = material.EnableTextureId(id);
        auto& texture = level_.GetTextureFromId(texture_id);
        if (texture.GetData().cubemap())
        {
            auto& gl_texture =
                dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
            gl_texture.Bind(p.second);
        }
        else
        {
            auto& gl_texture =
                dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
            gl_texture.Bind(p.second);
        }
        std::unique_ptr<UniformInterface> uniform_interface =
            std::make_unique<Uniform>(inner_name, p.second);
        program.AddUniform(std::move(uniform_interface));
    }

    glBindVertexArray(gl_mesh.GetId());

    auto& index_buffer = level_.GetBufferFromId(mesh.GetIndexBufferId());
    auto& gl_index_buffer = dynamic_cast<Buffer&>(index_buffer);
    // This was crashing the driver so...
    if (mesh.GetIndexSize())
    {
        gl_index_buffer.Bind();
        switch (mesh.GetData().render_primitive_enum())
        {
        case proto::NodeMesh::TRIANGLE_PRIMITIVE:
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeMesh::POINT_PRIMITIVE:
            glDrawElements(
                GL_POINTS,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeMesh::LINE_PRIMITIVE:
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        default:
            throw std::runtime_error(
                std::format(
                    "Couldn't draw primitive {}",
                    proto::NodeMesh_RenderPrimitiveEnum_Name(
                        mesh.GetData().render_primitive_enum())));
        }
        gl_index_buffer.UnBind();
    }
    program.UnUse();
    glBindVertexArray(0);

    for (const auto& [id, inner_name] : active_texture_bindings)
    {
        (void)inner_name;
        EntityId texture_id = id;
        if (level_.GetEnumTypeFromId(id) != EntityTypeEnum::TEXTURE)
        {
            texture_id = id + 1;
        }
        auto& texture = level_.GetTextureFromId(texture_id);
        if (texture.GetData().cubemap())
        {
            auto& gl_texture =
                dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
            gl_texture.UnBind();
        }
        else
        {
            auto& gl_texture =
                dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
            gl_texture.UnBind();
        }
    }
    material.DisableAll();

    if (mesh.IsClearBuffer())
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer::PresentFinal()
{
    auto maybe_quad_id = level_.GetDefaultMeshQuadId();
    if (maybe_quad_id == NullId)
        throw std::runtime_error("No quad id.");
    auto& quad = level_.GetMeshFromId(maybe_quad_id);
    auto& program = level_.GetProgramFromId(display_program_id_);
    UniformCollectionWrapper uniform_collection_wrapper{};
    program.Use(uniform_collection_wrapper, &level_);
    auto& material = level_.GetMaterialFromId(display_material_id_);
    const auto active_texture_bindings =
        GetActiveTextureBindings(material, program);
    for (const auto& [id, inner_name] : active_texture_bindings)
    {
        auto& opengl_texture =
            dynamic_cast<Texture&>(level_.GetTextureFromId(id));
        const auto p = material.EnableTextureId(id);
        opengl_texture.Bind(p.second);
        std::unique_ptr<UniformInterface> uniform_interface =
            std::make_unique<Uniform>(inner_name, p.second);
        program.AddUniform(std::move(uniform_interface));
    }
    auto& gl_quad = dynamic_cast<Mesh&>(quad);
    glBindVertexArray(gl_quad.GetId());
    auto& index_buffer = level_.GetBufferFromId(quad.GetIndexBufferId());
    auto& gl_index_buffer = dynamic_cast<Buffer&>(index_buffer);

    gl_index_buffer.Bind();
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(quad.GetIndexSize()) / sizeof(std::int32_t),
        GL_UNSIGNED_INT,
        nullptr);
    gl_index_buffer.UnBind();

    program.UnUse();
    glBindVertexArray(0);

    for (const auto& [id, inner_name] : active_texture_bindings)
    {
        (void)inner_name;
        auto& opengl_texture =
            dynamic_cast<Texture&>(level_.GetTextureFromId(id));
        opengl_texture.UnBind();
    }
    material.DisableAll();
}

void Renderer::SetDepthTest(bool enable)
{
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::PreRender()
{
    render_time_ = proto::NodeMesh::PRE_RENDER_TIME;
    // This will ensure that it is only true once.
    auto first_render = std::exchange(first_render_, false);
    auto preprocess_entry = [&](
                                const std::pair<EntityId, EntityId>& p,
                                bool preprocess_every_frame) {
        auto& node = level_.GetSceneNodeFromId(p.first);
        if (node.GetLocalMesh())
        {
            auto& mesh = level_.GetMeshFromId(node.GetLocalMesh());
            auto* gl_skinned_mesh =
                dynamic_cast<SkinnedMesh*>(&mesh);
            if (gl_skinned_mesh)
            {
                UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
            }
        }
        if (!first_render && !preprocess_every_frame)
        {
            return;
        }

        auto material_id = p.second;
        auto temp_viewport = viewport_;
        // Query textures from the material.
        auto& material = level_.GetMaterialFromId(material_id);
        if (material.GetPreprocessProgramId())
        {
            auto saved_program = material.GetProgramId();
            auto preprocess_id = material.GetPreprocessProgramId();
            if (preprocess_id)
            {
                auto& preprocess_program =
                    level_.GetProgramFromId(preprocess_id);
                auto out_ids = preprocess_program.GetOutputTextureIds();
                if (out_ids.empty())
                {
                    texture_frame_.set_value(proto::TextureFrame::TEXTURE_2D);
                    material.SetProgramId(preprocess_id);
                    RenderNode(
                        p.first,
                        material_id,
                        kProjectionCubemap,
                        kViewsCubemap[0]);
                    material.SetProgramId(saved_program);
                    viewport_ = temp_viewport;
                    return;
                }
                auto& tex = level_.GetTextureFromId(*out_ids.begin());
                auto size = json::ParseSize(tex.GetData().size());
                viewport_ = glm::ivec4(0, 0, size.x, size.y);
                material.SetProgramId(preprocess_id);
                RenderNode(
                    p.first,
                    material_id,
                    kProjectionCubemap,
                    kViewsCubemap[0]);
                material.SetProgramId(saved_program);
                viewport_ = temp_viewport;
            }
            return;
        }
        auto ids = material.GetTextureIds();
        if (ids.empty())
        {
            // Mesh has no target texture: just render once to populate
            // buffers without touching the framebuffer.
            RenderNode(
                p.first, material_id, kProjectionCubemap, kViewsCubemap[0]);
            return;
        }
        auto& texture = level_.GetTextureFromId(ids[0]);
        auto size = json::ParseSize(texture.GetData().size());
        viewport_ = glm::ivec4(0, 0, size.x, size.y);
        if (texture.GetData().cubemap())
        {
            for (std::uint32_t i = 0; i < 6; ++i)
            {
                proto::TextureFrame texture_frame;
                texture_frame.set_value(
                    static_cast<proto::TextureFrame::Enum>(
                        proto::TextureFrame::CUBE_MAP_POSITIVE_X + i));
                SetCubeMapTarget(texture_frame);
                RenderNode(
                    p.first,
                    material_id,
                    kProjectionCubemap,
                    kViewsCubemap[i]);
            }
        }
        else
        {
            // Regular 2D texture target.
            texture_frame_.set_value(proto::TextureFrame::TEXTURE_2D);
            RenderNode(
                p.first, material_id, kProjectionCubemap, kViewsCubemap[0]);
        }
        viewport_ = temp_viewport;
    };
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::PRE_RENDER_TIME))
    {
        preprocess_entry(p, false);
    }
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::SCENE_RENDER_TIME))
    {
        if (!IsRaytracingSourceMaterial(level_, p.second))
        {
            continue;
        }
        preprocess_entry(p, true);
    }
    if (RaytraceSceneRequiresWorldSpaceBuffers(level_))
    {
        UpdateAggregateRaytraceSceneBuffers();
        return;
    }
    UpdateSourceInstanceRaytraceSceneBuffers();
}

void Renderer::RenderSkybox(const CameraInterface& camera)
{
    render_time_ = proto::NodeMesh::SKYBOX_RENDER_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::SKYBOX_RENDER_TIME))
    {
        auto maybe_model = RenderNode(
            p.first,
            p.second,
            camera.ComputeProjection(),
            camera.ComputeView());
        if (maybe_model)
        {
            env_map_model_ = *maybe_model;
        }
    }
}

void Renderer::RenderScene(const CameraInterface& camera)
{
    render_time_ = proto::NodeMesh::SCENE_RENDER_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::SCENE_RENDER_TIME))
    {
        if (IsRaytracingSourceMaterial(level_, p.second))
        {
            continue;
        }
        RenderNode(
            p.first,
            p.second,
            camera.ComputeProjection(),
            camera.ComputeView());
    }
}

void Renderer::PostProcess()
{
    render_time_ = proto::NodeMesh::POST_PROCESS_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::POST_PROCESS_TIME))
    {
        // Is it correct for projection and view? This is a post process?
        RenderNode(p.first, p.second, glm::mat4(1.0), glm::mat4(1.0));
    }
}

} // End namespace frame::opengl.
