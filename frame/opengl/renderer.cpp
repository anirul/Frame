#include "renderer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <format>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>
#include <stdexcept>

#include "frame/bvh.h"
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
        if (skinned_mesh->HasSkinning() ||
            skinned_mesh->HasRaytraceTriangleCallback())
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

void HashByteSamples(
    std::size_t& seed, const std::vector<std::uint8_t>& bytes)
{
    HashCombine(seed, bytes.size());
    if (bytes.empty())
    {
        return;
    }

    constexpr std::array<float, 8> kRatios = {
        0.0f, 0.11f, 0.23f, 0.37f, 0.53f, 0.67f, 0.83f, 1.0f};
    const std::size_t max_index = bytes.size() - 1;
    for (const float ratio : kRatios)
    {
        const auto index = static_cast<std::size_t>(
            ratio * static_cast<float>(max_index));
        HashCombine(seed, bytes[index]);
    }
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

        HashByteSamples(state_hash, triangle_buffer->GetRawData());
    }
    return state_hash;
}

constexpr std::size_t kRaytraceFloatsPerVertex = 12;
constexpr std::size_t kRaytraceTriangleVertexStrideBytes =
    sizeof(float) * kRaytraceFloatsPerVertex;

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
}

void Renderer::UpdateRaytraceBuffersIfNeeded(SkinnedMesh& skinned_mesh)
{
    const double skinning_time = skinned_mesh.GetSkinningTime(delta_time_);

    if (skinned_mesh.HasRaytraceTriangleCallback())
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
            }
        }
    }

    if (skinned_mesh.HasRaytraceBvhCallback())
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
            }
        }
    }
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
    if (gl_skinned_mesh && gl_skinned_mesh->HasSkinning())
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
    }
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
