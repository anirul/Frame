#include "frame/vulkan/device.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>

#include <stdexcept>
#include "absl/flags/flag.h"

#include "frame/bvh.h"
#include "frame/camera.h"
#include "frame/json/program_key.h"
#include "frame/level.h"
#include "frame/common/application.h"
#include "frame/node_mesh.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/command_resources.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/mesh_utils.h"
#include "frame/vulkan/output_image_resources.h"
#include "frame/vulkan/pipeline_resources.h"
#include "frame/vulkan/renderer.h"
#include "frame/vulkan/scene_state.h"
#include "frame/vulkan/scoped_timer.h"
#include "frame/vulkan/shader_compiler.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/sync_resources.h"
#include "frame/vulkan/texture.h"
#include "frame/vulkan/texture_resources.h"
#include "frame/vulkan/skinned_mesh.h"
#include "frame/proto/uniform.pb.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace frame::vulkan
{

namespace
{

constexpr std::uint32_t kHardwareRaytracingAsBinding = 32;

vk::BuildAccelerationStructureFlagsKHR GetHardwareRaytracingBuildFlags()
{
    return vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
           vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
}

const char* BoolToString(bool value)
{
    return value ? "on" : "off";
}

template <typename T>
T AlignUp(T value, T alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

vk::ShaderStageFlags ToShaderStageFlags(
    const frame::proto::ProgramBinding& binding)
{
    vk::ShaderStageFlags flags{};
    for (auto stage : binding.stages())
    {
        switch (stage)
        {
        case frame::proto::ProgramStage::VERTEX:
            flags |= vk::ShaderStageFlagBits::eVertex;
            break;
        case frame::proto::ProgramStage::FRAGMENT:
            flags |= vk::ShaderStageFlagBits::eFragment;
            break;
        case frame::proto::ProgramStage::COMPUTE:
            flags |= vk::ShaderStageFlagBits::eCompute;
            break;
        case frame::proto::ProgramStage::INVALID_STAGE:
        default:
            break;
        }
    }
    return flags;
}

std::unordered_set<std::string> GetSupportedDeviceExtensions(
    vk::PhysicalDevice physical_device)
{
    std::unordered_set<std::string> supported = {};
    for (const auto& extension :
         physical_device.enumerateDeviceExtensionProperties())
    {
        supported.emplace(
            static_cast<const char*>(extension.extensionName));
    }
    return supported;
}

bool HasRequiredHardwareRaytracingExtensions(
    const std::unordered_set<std::string>& extensions)
{
    return extensions.contains(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
           extensions.contains(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
           extensions.contains(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
           extensions.contains(VK_KHR_SPIRV_1_4_EXTENSION_NAME) &&
           extensions.contains(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) &&
           extensions.contains(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
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

        if (include_node_matrices)
        {
            HashMatrix(state_hash, node->GetLocalModel(time_seconds));
        }
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
    frame::LevelInterface& level,
    double time_seconds)
{
    const auto source_mesh_materials = GetRaytracingSourceMeshMaterials(level);
    if (source_mesh_materials.empty())
    {
        return std::nullopt;
    }

    std::optional<glm::mat4> shared_model = std::nullopt;
    for (const auto& [source_node_id, source_material_id] : source_mesh_materials)
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
    frame::LevelInterface& level,
    double time_seconds)
{
    return !RaytraceSceneRequiresWorldSpaceBuffers(level) &&
           GetSharedRaytraceSceneTransform(level, time_seconds).has_value();
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
            "Animated Vulkan raytrace triangle buffer size is not aligned to the expected vertex stride.");
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
            "Animated Vulkan raytrace triangle buffer size is not aligned to the expected vertex stride.");
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

vk::TransformMatrixKHR MakeIdentityTransform()
{
    vk::TransformMatrixKHR transform = {};
    transform.matrix[0][0] = 1.0f;
    transform.matrix[1][1] = 1.0f;
    transform.matrix[2][2] = 1.0f;
    return transform;
}

struct alignas(16) HardwareRaytraceInstanceStorageData
{
    glm::mat4 object_to_world = glm::mat4(1.0f);
    glm::mat4 world_to_object = glm::mat4(1.0f);
    glm::uvec4 metadata = glm::uvec4(0u);
};

struct RaytracingSourceGeometryData
{
    frame::EntityId source_node_id = frame::NullId;
    frame::EntityId triangle_buffer_id = frame::NullId;
    frame::EntityId source_material_id = frame::NullId;
    std::uint32_t material_id = 0;
    std::uint32_t triangle_offset = 0;
    std::vector<std::uint8_t> triangle_bytes = {};
};

glm::mat4 InverseOrIdentity(const glm::mat4& matrix)
{
    const float determinant = glm::determinant(glm::mat3(matrix));
    if (std::abs(determinant) <= 1.0e-8f)
    {
        return glm::mat4(1.0f);
    }
    return glm::inverse(matrix);
}

vk::TransformMatrixKHR MakeAccelerationStructureTransform(const glm::mat4& matrix)
{
    vk::TransformMatrixKHR transform = {};
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            transform.matrix[row][column] = matrix[column][row];
        }
    }
    return transform;
}

std::optional<RaytracingSourceGeometryData> BuildRaytracingSourceGeometryDataForSource(
    frame::LevelInterface& level,
    frame::EntityId source_node_id,
    frame::EntityId source_material_id,
    double time_seconds,
    bool apply_node_transform,
    std::uint32_t triangle_offset)
{
    const auto transmissive =
        IsTransmissiveMaterial(level, source_material_id);
    const auto reference_color = ResolveRaytracingReferenceColor(
        level,
        transmissive);

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

    const auto source_color = ResolveRaytracingSourceMaterialColor(
        level,
        source_material_id);
    const auto color_multiplier = ResolveRaytracingColorMultiplier(
        source_color,
        reference_color);
    const auto transformed =
        apply_node_transform
            ? TransformTriangleBytes(
                  triangle_buffer->GetRawData(),
                  node->GetLocalModel(time_seconds))
            : triangle_buffer->GetRawData();

    RaytracingSourceGeometryData geometry = {};
    geometry.source_node_id = source_node_id;
    geometry.triangle_buffer_id = triangle_buffer_id;
    geometry.source_material_id = source_material_id;
    geometry.material_id = transmissive ? 0u : 1u;
    geometry.triangle_offset = triangle_offset;
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
        const std::uint32_t triangle_offset = transmissive
            ? next_transmissive_triangle_offset
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

bool AreTexturesCompatibleForReuse(
    const frame::vulkan::Texture& old_texture,
    const frame::vulkan::Texture& new_texture)
{
    const auto& old_data = old_texture.GetData();
    const auto& new_data = new_texture.GetData();

    if (!old_texture.HasGpuResources())
    {
        return false;
    }
    if (old_texture.GetViewType() != new_texture.GetViewType())
    {
        return false;
    }
    if (old_data.texture_oneof_case() != new_data.texture_oneof_case())
    {
        return false;
    }
    if (old_data.has_file_name() != new_data.has_file_name())
    {
        return false;
    }
    if (old_data.has_file_name() &&
        old_data.file_name() != new_data.file_name())
    {
        return false;
    }
    if (old_data.has_file_names() != new_data.has_file_names())
    {
        return false;
    }
    if (old_data.has_file_names())
    {
        const auto& old_files = old_data.file_names();
        const auto& new_files = new_data.file_names();
        if (old_files.positive_x() != new_files.positive_x() ||
            old_files.negative_x() != new_files.negative_x() ||
            old_files.positive_y() != new_files.positive_y() ||
            old_files.negative_y() != new_files.negative_y() ||
            old_files.positive_z() != new_files.positive_z() ||
            old_files.negative_z() != new_files.negative_z())
        {
            return false;
        }
    }
    // Pixel-backed textures may change without size/format changes.
    // Force re-upload for those cases.
    if (old_data.has_pixels() || new_data.has_pixels())
    {
        return false;
    }
    if (old_data.has_min_filter() != new_data.has_min_filter())
    {
        return false;
    }
    if (old_data.has_min_filter() &&
        old_data.min_filter().value() != new_data.min_filter().value())
    {
        return false;
    }
    if (old_data.has_mag_filter() != new_data.has_mag_filter())
    {
        return false;
    }
    if (old_data.has_mag_filter() &&
        old_data.mag_filter().value() != new_data.mag_filter().value())
    {
        return false;
    }
    if (old_data.has_wrap_s() != new_data.has_wrap_s())
    {
        return false;
    }
    if (old_data.has_wrap_s() &&
        old_data.wrap_s().value() != new_data.wrap_s().value())
    {
        return false;
    }
    if (old_data.has_wrap_t() != new_data.has_wrap_t())
    {
        return false;
    }
    if (old_data.has_wrap_t() &&
        old_data.wrap_t().value() != new_data.wrap_t().value())
    {
        return false;
    }
    if (old_data.mipmap() != new_data.mipmap())
    {
        return false;
    }
    if (old_data.has_size() != new_data.has_size())
    {
        return false;
    }
    if (old_data.has_size())
    {
        if (old_data.size().x() != new_data.size().x() ||
            old_data.size().y() != new_data.size().y())
        {
            return false;
        }
    }
    return old_data.pixel_element_size().value() ==
               new_data.pixel_element_size().value() &&
           old_data.pixel_structure().value() ==
               new_data.pixel_structure().value();
}

std::size_t TransferTextureGpuResources(
    frame::LevelInterface& old_level,
    frame::LevelInterface& new_level)
{
    std::size_t transfer_count = 0;
    for (const auto new_texture_id : new_level.GetTextures())
    {
        const std::string texture_name = new_level.GetNameFromId(new_texture_id);
        const auto old_texture_id = old_level.GetIdFromName(texture_name);
        if (old_texture_id == frame::NullId)
        {
            continue;
        }
        auto* old_texture = dynamic_cast<frame::vulkan::Texture*>(
            &old_level.GetTextureFromId(old_texture_id));
        auto* new_texture = dynamic_cast<frame::vulkan::Texture*>(
            &new_level.GetTextureFromId(new_texture_id));
        if (!old_texture || !new_texture)
        {
            continue;
        }
        if (!AreTexturesCompatibleForReuse(*old_texture, *new_texture))
        {
            continue;
        }
        if (old_texture->MoveGpuResourcesTo(*new_texture))
        {
            ++transfer_count;
        }
    }
    return transfer_count;
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

void UploadDeviceLocalBuffer(
    vk::Device device,
    frame::vulkan::GpuMemoryManager& gpu_memory_manager,
    frame::vulkan::CommandQueue& command_queue,
    const std::vector<std::uint8_t>& bytes,
    vk::Buffer destination)
{
    if (bytes.empty())
    {
        return;
    }

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = gpu_memory_manager.CreateBuffer(
        static_cast<vk::DeviceSize>(bytes.size()),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);
    void* mapped = device.mapMemory(
        *staging_memory, 0, static_cast<vk::DeviceSize>(bytes.size()));
    std::memcpy(mapped, bytes.data(), bytes.size());
    device.unmapMemory(*staging_memory);
    command_queue.CopyBuffer(
        *staging_buffer,
        destination,
        static_cast<vk::DeviceSize>(bytes.size()));
}

std::vector<std::uint8_t> BuildAggregateTriangleBytes(
    frame::LevelInterface& level,
    bool transmissive,
    double time_seconds,
    bool apply_node_transform = true)
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
            dynamic_cast<frame::NodeMesh*>(
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
        const auto color_multiplier = ResolveRaytracingColorMultiplier(
            source_color,
            reference_color);
        const auto transformed =
            apply_node_transform
                ? TransformTriangleBytes(
                      triangle_buffer->GetRawData(),
                      node->GetLocalModel(time_seconds))
                : triangle_buffer->GetRawData();
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
            "Animated raytrace triangle buffer size is not aligned to the expected vertex stride.");
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

Device::Device(
    void* vk_instance,
    glm::uvec2 size,
    vk::SurfaceKHR& surface)
    : vk_instance_(static_cast<VkInstance>(vk_instance)),
      size_(size),
      vk_surface_(surface),
      texture_resources_(std::make_unique<TextureResources>(*this)),
      pipeline_resources_(std::make_unique<PipelineResources>(*this)),
      output_image_resources_(std::make_unique<OutputImageResources>(*this)),
      renderer_(std::make_unique<Renderer>(*this))
{
    logger_->info("Initializing Vulkan device ({}x{})", size_.x, size_.y);

    std::vector<vk::PhysicalDevice> physical_devices =
        vk_instance_.enumeratePhysicalDevices();
    if (physical_devices.empty())
    {
        throw std::runtime_error("No Vulkan physical device found.");
    }

    int best_score = std::numeric_limits<int>::min();
    for (const auto& physical_device : physical_devices)
    {
        const auto properties = physical_device.getProperties();
        const auto features = physical_device.getFeatures();

        int score = 0;
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            score += 1000;
        }
        score += static_cast<int>(properties.limits.maxImageDimension2D);
        if (!features.geometryShader)
        {
            continue;
        }

        const std::string device_name(properties.deviceName.data());
        logger_->info("Evaluated Vulkan device: {}", device_name);
        if (score > best_score)
        {
            best_score = score;
            vk_physical_device_ = physical_device;
        }
    }

    if (!vk_physical_device_)
    {
        throw std::runtime_error(
            "No suitable Vulkan physical device with geometry shader support.");
    }

    const auto queue_families = vk_physical_device_.getQueueFamilyProperties();
    std::optional<std::uint32_t> graphics_queue_index;
    std::optional<std::uint32_t> present_queue_index;

    for (std::uint32_t index = 0; index < queue_families.size(); ++index)
    {
        const auto& queue_family = queue_families[index];

        const bool supports_graphics =
            (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) ==
            vk::QueueFlagBits::eGraphics;
        if (supports_graphics && !graphics_queue_index)
        {
            graphics_queue_index = index;
        }

        const bool supports_present =
            vk_physical_device_.getSurfaceSupportKHR(index, vk_surface_);
        if (supports_present && !present_queue_index)
        {
            present_queue_index = index;
        }

        if (graphics_queue_index && present_queue_index)
        {
            break;
        }
    }

    if (!graphics_queue_index)
    {
        throw std::runtime_error("No Vulkan queue family supporting graphics.");
    }

    if (!present_queue_index)
    {
        if (vk_physical_device_.getSurfaceSupportKHR(
                graphics_queue_index.value(),
                vk_surface_))
        {
            present_queue_index = graphics_queue_index;
        }
        else
        {
            throw std::runtime_error(
                "No Vulkan queue family supporting presentation.");
        }
    }

    graphics_queue_family_index_ = graphics_queue_index.value();
    present_queue_family_index_ = present_queue_index.value();

    std::vector<std::uint32_t> unique_queue_indices = {graphics_queue_family_index_};
    if (present_queue_family_index_ != graphics_queue_family_index_)
    {
        unique_queue_indices.push_back(present_queue_family_index_);
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(unique_queue_indices.size());
    for (auto index : unique_queue_indices)
    {
        queue_create_infos.emplace_back(
            vk::DeviceQueueCreateFlags{},
            index,
            1,
            &queue_family_priority_);
    }

    const auto supported_extensions =
        GetSupportedDeviceExtensions(vk_physical_device_);
    hardware_raytracing_supported_ =
        HasRequiredHardwareRaytracingExtensions(supported_extensions);
    if (hardware_raytracing_supported_)
    {
        vk::PhysicalDeviceFeatures2 features2{};
        features2.setPNext(&buffer_device_address_features_);
        buffer_device_address_features_.setPNext(
            &acceleration_structure_features_);
        acceleration_structure_features_.setPNext(
            &raytracing_pipeline_features_);
        vk_physical_device_.getFeatures2(&features2);
        hardware_raytracing_supported_ =
            buffer_device_address_features_.bufferDeviceAddress &&
            acceleration_structure_features_.accelerationStructure &&
            raytracing_pipeline_features_.rayTracingPipeline;
        if (hardware_raytracing_supported_)
        {
            vk::PhysicalDeviceProperties2 properties2{};
            properties2.pNext = &raytracing_pipeline_properties_;
            vk_physical_device_.getProperties2(&properties2);
        }
    }

    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    if (hardware_raytracing_supported_)
    {
        device_extensions.push_back(
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        device_extensions.push_back(
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        device_extensions.push_back(
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        device_extensions.push_back(
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }

    const auto supported_features = vk_physical_device_.getFeatures();
    vk::PhysicalDeviceFeatures device_features{};
    device_features.geometryShader = supported_features.geometryShader;
    device_features.samplerAnisotropy = supported_features.samplerAnisotropy;
    device_features.shaderStorageImageExtendedFormats =
        supported_features.shaderStorageImageExtendedFormats;
    if (!device_features.shaderStorageImageExtendedFormats &&
        compute_output_format_ == vk::Format::eR16G16B16A16Sfloat)
    {
        logger_->warn(
            "shaderStorageImageExtendedFormats not supported; "
            "raytracing compute output uses rgba16f storage images and may fail.");
    }

    vk::DeviceCreateInfo device_create_info(
        {},
        static_cast<std::uint32_t>(queue_create_infos.size()),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<std::uint32_t>(device_extensions.size()),
        device_extensions.data());
    device_create_info.setPEnabledFeatures(&device_features);
    if (hardware_raytracing_supported_)
    {
        buffer_device_address_features_.bufferDeviceAddress = VK_TRUE;
        acceleration_structure_features_.accelerationStructure = VK_TRUE;
        raytracing_pipeline_features_.rayTracingPipeline = VK_TRUE;
        buffer_device_address_features_.setPNext(
            &acceleration_structure_features_);
        acceleration_structure_features_.setPNext(
            &raytracing_pipeline_features_);
        device_create_info.setPNext(&buffer_device_address_features_);
    }

    vk_unique_device_ = vk_physical_device_.createDeviceUnique(device_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*vk_unique_device_);
    graphics_queue_ = vk_unique_device_->getQueue(graphics_queue_family_index_, 0);
    present_queue_ = vk_unique_device_->getQueue(present_queue_family_index_, 0);

    logger_->info(
        "Vulkan logical device created (graphics queue family {}, present queue family {}).",
        graphics_queue_family_index_,
        present_queue_family_index_);
    if (hardware_raytracing_supported_)
    {
        logger_->info("Vulkan hardware raytracing pipeline extensions enabled.");
    }
    else
    {
        logger_->info(
            "Vulkan hardware raytracing pipeline unavailable; using software compute traversal.");
    }
}

Device::~Device()
{
    Shutdown();
}

void Device::SetStereo(
    StereoEnum stereo_enum,
    float interocular_distance,
    glm::vec3 focus_point,
    bool invert_left_right)
{
    stereo_enum_ = stereo_enum;
    interocular_distance_ = interocular_distance;
    focus_point_ = focus_point;
    invert_left_right_ = invert_left_right;
}

void Device::Clear(const glm::vec4& /*color*/) const
{
    // TODO: hook up Vulkan render passes once the swapchain exists.
}

void Device::Startup(std::unique_ptr<LevelInterface>&& level)
{
    level_ = std::move(level);
}

void Device::StartupFromLevelData(const frame::json::LevelData& level_data)
{
    ScopedTimer total_timer(logger_, "Vulkan StartupFromLevelData");

    current_level_data_ = level_data;
    active_program_info_.reset();
    use_compute_raytracing_ = false;
    use_raytracing_pipeline_ = false;
    elapsed_time_seconds_ = 0.0f;
    last_raytrace_scene_state_hash_ = 0;
    has_raytrace_scene_state_hash_ = false;

    // Prefer programs configured for render passes; otherwise fall back to the
    // first available program.
    {
        ScopedTimer timer(logger_, "Select program");
        auto pick_program = [&]() -> std::optional<frame::json::ProgramInfo> {
            auto find_program_by_name =
                [&](const std::string& program_name)
                -> std::optional<frame::json::ProgramInfo> {
                    if (program_name.empty())
                    {
                        return std::nullopt;
                    }
                    for (const auto& program : level_data.programs)
                    {
                        if (program.name == program_name)
                        {
                            return program;
                        }
                    }
                    return std::nullopt;
                };
            auto find_program_info_by_name =
                [&](const std::string& program_name)
                -> const frame::json::ProgramInfo* {
                    if (program_name.empty())
                    {
                        return nullptr;
                    }
                    for (const auto& program_info : level_data.programs)
                    {
                        if (program_info.name == program_name)
                        {
                            return &program_info;
                        }
                    }
                    return nullptr;
                };
            auto is_raytracing_program =
                [&](const std::string& program_name) {
                    const auto* program_info =
                        find_program_info_by_name(program_name);
                    if (!program_info)
                    {
                        return false;
                    }
                    const auto key =
                        frame::json::ResolveProgramKey(program_info->proto);
                    return frame::json::IsRaytracingProgramKey(key);
                };
            auto pass_has_renderable_mesh =
                [&](frame::proto::NodeMesh::RenderTimeEnum pass) {
                    if (!level_data.proto.has_scene_tree())
                    {
                        return false;
                    }
                    for (const auto& node_mesh :
                         level_data.proto.scene_tree().node_meshes())
                    {
                        if (node_mesh.render_time_enum() != pass)
                        {
                            continue;
                        }
                        // Clean-buffer nodes do not contribute renderable
                        // geometry for pass selection.
                        if (node_mesh.has_clean_buffer())
                        {
                            continue;
                        }
                        return true;
                    }
                    return false;
                };

            constexpr std::array<frame::proto::NodeMesh::RenderTimeEnum, 4>
                kPreferredPasses = {
                    frame::proto::NodeMesh::SCENE_RENDER_TIME,
                    frame::proto::NodeMesh::PRE_RENDER_TIME,
                    frame::proto::NodeMesh::POST_PROCESS_TIME,
                    frame::proto::NodeMesh::SKYBOX_RENDER_TIME};
            bool has_renderable_mesh_in_preferred_pass = false;
            for (const auto pass : kPreferredPasses)
            {
                if (pass_has_renderable_mesh(pass))
                {
                    has_renderable_mesh_in_preferred_pass = true;
                    break;
                }
            }
            std::optional<frame::json::ProgramInfo> first_pass_program =
                std::nullopt;
            for (const auto pass : kPreferredPasses)
            {
                if (has_renderable_mesh_in_preferred_pass &&
                    !pass_has_renderable_mesh(pass))
                {
                    continue;
                }
                for (const auto& pass_program :
                     level_data.render_pass_programs)
                {
                    if (pass_program.render_time != pass)
                    {
                        continue;
                    }
                    if (auto program =
                            find_program_by_name(pass_program.program_name))
                    {
                        if (!first_pass_program)
                        {
                            first_pass_program = program;
                        }
                        if (is_raytracing_program(pass_program.program_name))
                        {
                            return program;
                        }
                    }
                }
            }
            if (first_pass_program)
            {
                return first_pass_program;
            }
            for (const auto& program_info : level_data.programs)
            {
                if (is_raytracing_program(program_info.name))
                {
                    return program_info;
                }
            }
            if (!level_data.programs.empty())
            {
                return level_data.programs.front();
            }
            return std::nullopt;
        };

        if (auto chosen_program = pick_program())
        {
            ProgramPipelineInfo pipeline_info;
            const auto& program_info = *chosen_program;
            pipeline_info.program_name = program_info.name;
            const auto shader_root =
                level_data.asset_root / "shader" / "vulkan";
            pipeline_info.vertex_shader =
                shader_root / program_info.vulkan.vertex_shader;
            pipeline_info.fragment_shader =
                shader_root / program_info.vulkan.fragment_shader;
            pipeline_info.use_compute =
                !program_info.vulkan.compute_shader.empty();
            if (pipeline_info.use_compute)
            {
                pipeline_info.compute_shader =
                    shader_root / program_info.vulkan.compute_shader;
                if (!program_info.vulkan.raygen_shader.empty())
                {
                    pipeline_info.raygen_shader =
                        shader_root / program_info.vulkan.raygen_shader;
                }
                if (!program_info.vulkan.miss_shader.empty())
                {
                    pipeline_info.miss_shader =
                        shader_root / program_info.vulkan.miss_shader;
                }
                if (!program_info.vulkan.closesthit_shader.empty())
                {
                    pipeline_info.closesthit_shader =
                        shader_root / program_info.vulkan.closesthit_shader;
                }
            }
            pipeline_info.scene_type =
                program_info.proto.input_scene_type().value();
            for (const auto& uniform : program_info.proto.uniforms())
            {
                if (uniform.value_oneof_case() ==
                        frame::proto::Uniform::kUniformEnum &&
                    uniform.uniform_enum() ==
                        frame::proto::Uniform::FLOAT_TIME_S)
                {
                    pipeline_info.uses_time_uniform = true;
                }
            }
            pipeline_info.bindings.clear();
            pipeline_info.bindings.reserve(
                static_cast<std::size_t>(program_info.proto.bindings_size()));
            for (const auto& binding : program_info.proto.bindings())
            {
                ProgramPipelineInfo::BindingInfo binding_info;
                binding_info.name = binding.name();
                binding_info.binding = binding.binding();
                binding_info.binding_type = binding.binding_type();
                binding_info.stages = ToShaderStageFlags(binding);
                pipeline_info.bindings.push_back(std::move(binding_info));
            }
            active_program_info_ = std::move(pipeline_info);
            logger_->info(
                "Vulkan startup selected active program '{}'.",
                active_program_info_->program_name);
        }
        else
        {
            logger_->error(
                "No Vulkan program selected from the internal render setup.");
        }
    }

    auto previous_level = std::move(level_);
    std::size_t transferred_texture_count = 0;
    const bool prefer_hardware_raytracing_build =
        active_program_info_ &&
        active_program_info_->use_compute &&
        hardware_raytracing_supported_ &&
        !active_program_info_->raygen_shader.empty() &&
        !active_program_info_->miss_shader.empty() &&
        !active_program_info_->closesthit_shader.empty();
    {
        ScopedTimer timer(logger_, "BuildLevel");
        auto built = BuildLevel(
            GetSize(),
            level_data,
            {.prefer_hardware_raytracing = prefer_hardware_raytracing_build});
        if (previous_level && built.level)
        {
            transferred_texture_count =
                TransferTextureGpuResources(*previous_level, *built.level);
        }
        level_ = std::move(built.level);
    }
    if (transferred_texture_count > 0)
    {
        logger_->info(
            "Reused {} Vulkan texture GPU resources.",
            transferred_texture_count);
    }

    if (active_program_info_ && level_)
    {
        active_program_info_->program_id =
            level_->GetIdFromName(active_program_info_->program_name);
        use_compute_raytracing_ = active_program_info_->use_compute;
        active_program_info_->texture_ids_by_inner.clear();
        active_program_info_->buffer_ids_by_inner.clear();
        active_program_info_->material_id = NullId;
        if (active_program_info_->program_id != NullId)
        {
            try
            {
                std::vector<EntityId> candidate_material_ids = {};
                std::unordered_set<EntityId> candidate_material_seen = {};
                std::unordered_set<EntityId> mesh_bound_material_ids = {};
                auto append_mesh_material_candidates =
                    [&](frame::proto::NodeMesh::RenderTimeEnum render_time) {
                        if (level_->GetRenderPassProgramId(render_time) !=
                            active_program_info_->program_id)
                        {
                            return;
                        }
                        for (const auto& pair :
                             level_->GetMeshMaterialIds(render_time))
                        {
                            const auto material_id = pair.second;
                            if (material_id == NullId)
                            {
                                continue;
                            }
                            mesh_bound_material_ids.insert(material_id);
                            if (candidate_material_seen.insert(material_id).second)
                            {
                                candidate_material_ids.push_back(material_id);
                            }
                        }
                    };
                append_mesh_material_candidates(
                    frame::proto::NodeMesh::SCENE_RENDER_TIME);
                append_mesh_material_candidates(
                    frame::proto::NodeMesh::PRE_RENDER_TIME);
                append_mesh_material_candidates(
                    frame::proto::NodeMesh::POST_PROCESS_TIME);
                append_mesh_material_candidates(
                    frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
                for (const auto material_id : level_->GetMaterials())
                {
                    if (candidate_material_seen.insert(material_id).second)
                    {
                        candidate_material_ids.push_back(material_id);
                    }
                }

                EntityId selected_material_id = NullId;
                int selected_material_score = std::numeric_limits<int>::min();
                const auto count_matching_bindings =
                    [&](const frame::MaterialInterface& material) {
                        int matches = 0;
                        for (const auto& binding : active_program_info_->bindings)
                        {
                            if (binding.name.empty())
                            {
                                continue;
                            }
                            switch (binding.binding_type)
                            {
                            case frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER: {
                                for (const auto texture_id : material.GetTextureIds())
                                {
                                    if (material.GetInnerName(texture_id) ==
                                        binding.name)
                                    {
                                        ++matches;
                                        break;
                                    }
                                }
                                break;
                            }
                            case frame::proto::ProgramBinding::STORAGE_BUFFER: {
                                for (const auto& buffer_name : material.GetBufferNames())
                                {
                                    if (material.GetInnerBufferName(buffer_name) ==
                                        binding.name)
                                    {
                                        ++matches;
                                        break;
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                            }
                        }
                        return matches;
                    };
                for (const auto material_id : candidate_material_ids)
                {
                    auto& material = level_->GetMaterialFromId(material_id);
                    if (material.GetProgramId(level_.get()) !=
                        active_program_info_->program_id)
                    {
                        continue;
                    }
                    const int matching_bindings =
                        count_matching_bindings(material);
                    const int score =
                        matching_bindings * 10000 +
                        (mesh_bound_material_ids.contains(material_id)
                             ? 1000
                             : 0) +
                        static_cast<int>(material.GetTextureIds().size()) +
                        static_cast<int>(material.GetBufferNames().size());
                    if (score > selected_material_score)
                    {
                        selected_material_score = score;
                        selected_material_id = material_id;
                    }
                }

                if (selected_material_id != NullId)
                {
                    auto& material =
                        level_->GetMaterialFromId(selected_material_id);
                    active_program_info_->material_id = selected_material_id;
                    for (auto texture_id : material.GetTextureIds())
                    {
                        const auto inner_name = material.GetInnerName(texture_id);
                        if (!inner_name.empty())
                        {
                            active_program_info_->texture_ids_by_inner[inner_name] =
                                texture_id;
                        }
                    }
                    for (const auto& buffer_name : material.GetBufferNames())
                    {
                        const auto inner_name =
                            material.GetInnerBufferName(buffer_name);
                        if (inner_name.empty())
                        {
                            continue;
                        }
                        const auto buffer_id = level_->GetIdFromName(buffer_name);
                        if (buffer_id != NullId)
                        {
                            active_program_info_->buffer_ids_by_inner[inner_name] =
                                buffer_id;
                        }
                    }
                    logger_->info(
                        "Selected Vulkan material '{}' for program '{}' (score {}).",
                        level_->GetNameFromId(selected_material_id),
                        active_program_info_->program_name,
                        selected_material_score);
                }
            }
            catch (const std::exception& ex)
            {
                logger_->warn(
                    "Failed to gather material bindings for program {}: {}",
                    active_program_info_->program_name,
                    ex.what());
            }
        }
        if (active_program_info_->material_id == NullId)
        {
            logger_->warn(
                "No material found for Vulkan program {}.",
                active_program_info_->program_name);
        }
        if (active_program_info_ && active_program_info_->use_compute)
        {
            use_hardware_raytracing_ = false;
            use_raytracing_pipeline_ = false;
            if (hardware_raytracing_supported_ &&
                !active_program_info_->raygen_shader.empty() &&
                !active_program_info_->miss_shader.empty() &&
                !active_program_info_->closesthit_shader.empty())
            {
                use_hardware_raytracing_ = true;
                use_raytracing_pipeline_ = true;
                logger_->info(
                    "Using Vulkan hardware raytracing pipeline for program {}.",
                    active_program_info_->program_name);
            }
            else if (hardware_raytracing_supported_)
            {
                logger_->info(
                    "Vulkan hardware raytracing is available but program {} has no ray tracing stage mapping.",
                    active_program_info_->program_name);
            }
        }
        if (active_program_info_ && active_program_info_->use_compute &&
            active_program_info_->compute_shader.empty() &&
            current_level_data_)
        {
            logger_->warn(
                "Compute program {} has no mapped compute pipeline source.",
                active_program_info_->program_name);
        }
    }

    if (!command_resources_)
    {
        command_resources_ = std::make_unique<CommandResources>(
            *vk_unique_device_, graphics_queue_family_index_);
        command_resources_->Create();
        gpu_memory_manager_ = std::make_unique<GpuMemoryManager>(
            vk_physical_device_, *vk_unique_device_);
        command_queue_ = std::make_unique<CommandQueue>(
            *vk_unique_device_,
            graphics_queue_,
            *command_resources_->GetPool());
        buffer_resources_ = std::make_unique<BufferResourceManager>(
            *vk_unique_device_,
            *gpu_memory_manager_,
            *command_queue_,
            logger_);
        mesh_resources_ = std::make_unique<MeshResources>(
            *vk_unique_device_,
            *gpu_memory_manager_,
            *command_queue_,
            logger_);
    }
    if (!swapchain_resources_)
    {
        swapchain_resources_ = std::make_unique<SwapchainResources>(
            vk_physical_device_,
            *vk_unique_device_,
            vk_surface_,
            graphics_queue_family_index_,
            present_queue_family_index_,
            logger_);
    }
    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    DestroyRaytracingPipeline();
    DestroyComputePipeline();
    DestroyGraphicsPipeline();
    DestroyDescriptorResources();
    DestroyHardwareRaytracingScene();
    // TextureResources keeps raw pointers to textures owned by the previous level.
    // Clear the resource map after resource transfer and before dropping old level.
    DestroyTextureResources();
    if (mesh_resources_)
    {
        mesh_resources_->Clear();
    }

    try
    {
        {
            ScopedTimer timer(logger_, "CreateTextureResources");
            CreateTextureResources(level_data);
        }
        if (use_hardware_raytracing_)
        {
            ScopedTimer timer(logger_, "CreateHardwareRaytracingScene");
            CreateHardwareRaytracingScene();
        }
        {
            ScopedTimer timer(logger_, "CreateSwapchainResources");
            if (swapchain_resources_ && !swapchain_resources_->IsValid())
            {
                swapchain_resources_->Create(size_);
            }
            if (command_resources_ && command_resources_->GetBuffers().empty())
            {
                command_resources_->AllocateBuffers(
                    static_cast<std::uint32_t>(kMaxFramesInFlight));
            }
        }
        {
            ScopedTimer timer(logger_, "CreateSwapchainPreviewImage");
            CreateSwapchainPreviewImage();
        }
        {
            ScopedTimer timer(logger_, "CreateDescriptorResources");
            CreateDescriptorResources();
        }
        if (mesh_resources_)
        {
            ScopedTimer timer(logger_, "MeshResources Build");
            mesh_resources_->Build(level_data);
        }
        {
            ScopedTimer timer(logger_, "CreateGraphicsPipeline");
            CreateGraphicsPipeline();
        }
        if (use_raytracing_pipeline_)
        {
            ScopedTimer timer(logger_, "CreateRaytracingPipeline");
            CreateRaytracingPipeline();
        }
        else if (use_compute_raytracing_)
        {
            ScopedTimer timer(logger_, "CreateComputePipeline");
            CreateComputePipeline();
        }
        LogRuntimeConfiguration();
    }
    catch (const std::exception& ex)
    {
        logger_->error("Failed to prepare Vulkan GPU resources: {}", ex.what());
        DestroyDescriptorResources();
        DestroyHardwareRaytracingScene();
        DestroyTextureResources();
        if (mesh_resources_)
        {
            mesh_resources_->Clear();
        }
        DestroyGraphicsPipeline();
        DestroyRaytracingPipeline();
        DestroyComputePipeline();
    }

    if (!sync_resources_)
    {
        sync_resources_ = std::make_unique<SyncResources>(
            *vk_unique_device_,
            kMaxFramesInFlight,
            swapchain_resources_ ? swapchain_resources_->GetImages().size() : 0);
    }
    else if (swapchain_resources_ &&
             sync_resources_->GetSwapchainImageCount() !=
                 swapchain_resources_->GetImages().size())
    {
        sync_resources_->Destroy();
        sync_resources_->SetSwapchainImageCount(
            swapchain_resources_->GetImages().size());
    }
    if (sync_resources_ && !sync_resources_->IsCreated())
    {
        sync_resources_->Create();
    }

    previous_level.reset();
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface)
{
    if (!plugin_interface)
    {
        return;
    }
    plugin_interfaces_.push_back(std::move(plugin_interface));
}

void Device::SetGuiRenderCallback(GuiRenderCallback callback)
{
    gui_render_callback_ = std::move(callback);
}

void Device::ClearGuiRenderCallback()
{
    gui_render_callback_ = nullptr;
}

std::vector<PluginInterface*> Device::GetPluginPtrs()
{
    std::vector<PluginInterface*> plugins;
    plugins.reserve(plugin_interfaces_.size());
    for (auto& plugin : plugin_interfaces_)
    {
        plugins.push_back(plugin.get());
    }
    return plugins;
}

std::vector<std::string> Device::GetPluginNames() const
{
    std::vector<std::string> names;
    names.reserve(plugin_interfaces_.size());
    for (const auto& plugin : plugin_interfaces_)
    {
        names.push_back(plugin->GetName());
    }
    return names;
}

void Device::RemovePluginByName(const std::string& name)
{
    std::erase_if(plugin_interfaces_, [&name](const auto& plugin) {
        return plugin && plugin->GetName() == name;
    });
}

void Device::Cleanup()
{
    if (renderer_)
    {
        renderer_->Reset();
    }
    if (vk_unique_device_)
    {
        const VkResult result =
            vkDeviceWaitIdle(static_cast<VkDevice>(*vk_unique_device_));
        if (result != VK_SUCCESS)
        {
            logger_->warn(
                "vkDeviceWaitIdle failed during cleanup: {}",
                vk::to_string(static_cast<vk::Result>(result)));
        }
    }

    gui_render_callback_ = nullptr;
    for (auto& plugin : plugin_interfaces_)
    {
        if (plugin)
        {
            plugin->End();
        }
    }
    plugin_interfaces_.clear();

    DestroyRaytracingPipeline();
    DestroyComputePipeline();
    DestroyGraphicsPipeline();
    DestroySwapchainPreviewImage();
    if (swapchain_resources_)
    {
        swapchain_resources_->Destroy();
    }
    DestroyDescriptorResources();
    DestroyHardwareRaytracingScene();
    DestroyTextureResources();
    if (mesh_resources_)
    {
        mesh_resources_->Clear();
    }
    command_queue_.reset();
    buffer_resources_.reset();
    mesh_resources_.reset();
    gpu_memory_manager_.reset();
    if (command_resources_)
    {
        command_resources_->Destroy();
        command_resources_.reset();
    }
    if (sync_resources_)
    {
        sync_resources_->Destroy();
    }
    current_level_data_.reset();
    level_.reset();
    elapsed_time_seconds_ = 0.0f;
    last_raytrace_scene_state_hash_ = 0;
    has_raytrace_scene_state_hash_ = false;
    active_program_info_.reset();
    use_raytracing_pipeline_ = false;
}

void Device::Resize(glm::uvec2 size)
{
    size_ = size;
    framebuffer_resized_ = true;
    for (auto& plugin : plugin_interfaces_)
    {
        if (plugin)
        {
            plugin->Startup(size_);
        }
    }
}

glm::uvec2 Device::GetSize() const
{
    return size_;
}

void Device::UpdateRaytraceBuffers()
{
    if (!level_ ||
        !buffer_resources_ ||
        (!use_compute_raytracing_ && !use_raytracing_pipeline_))
    {
        return;
    }

    auto sample_triangle_data = [](const std::vector<float>& triangles) {
        std::array<float, 6> sample = {};
        if (triangles.empty())
        {
            return sample;
        }
        const std::array<float, 6> ratios = {
            0.0f, 0.17f, 0.33f, 0.51f, 0.73f, 0.91f};
        const std::size_t max_index = triangles.size() - 1;
        for (std::size_t i = 0; i < ratios.size(); ++i)
        {
            const std::size_t index = static_cast<std::size_t>(
                ratios[i] * static_cast<float>(max_index));
            sample[i] = triangles[index];
        }
        return sample;
    };

    static std::unordered_map<EntityId, std::array<float, 6>> previous_samples;
    static bool logged_motion = false;
    std::size_t updated_buffer_count = 0;
    std::vector<EntityId> updated_source_triangle_buffer_ids = {};
    const bool use_incremental_hardware_rt_updates =
        use_hardware_raytracing_ && hardware_raytracing_uses_source_instances_;
    for (const auto node_id : level_->GetSceneNodes())
    {
        auto* node_mesh =
            dynamic_cast<frame::NodeMesh*>(&level_->GetSceneNodeFromId(node_id));
        if (!node_mesh)
        {
            continue;
        }
        const auto mesh_id = node_mesh->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        auto* skinned_mesh = dynamic_cast<frame::vulkan::SkinnedMesh*>(
            &level_->GetMeshFromId(mesh_id));
        if (!skinned_mesh)
        {
            continue;
        }
        const double skinning_time = skinned_mesh->GetSkinningTime(
            static_cast<double>(elapsed_time_seconds_));

        const auto triangle_buffer_id = skinned_mesh->GetTriangleBufferId();
        if (triangle_buffer_id &&
            skinned_mesh->HasActiveRaytraceTriangleCallback())
        {
            auto triangles = skinned_mesh->EvaluateRaytraceTriangles(skinning_time);
            if (!triangles.empty())
            {
                const auto sample = sample_triangle_data(triangles);
                if (!logged_motion)
                {
                    if (auto it = previous_samples.find(triangle_buffer_id);
                        it != previous_samples.end())
                    {
                        bool changed = false;
                        for (std::size_t i = 0; i < sample.size(); ++i)
                        {
                            if (std::abs(sample[i] - it->second[i]) > 1.0e-5f)
                            {
                                changed = true;
                                break;
                            }
                        }
                        if (changed)
                        {
                            logger_->info(
                                "Detected Vulkan skinned animation updates for mesh '{}'.",
                                level_->GetNameFromId(mesh_id));
                            logged_motion = true;
                        }
                    }
                    previous_samples[triangle_buffer_id] = sample;
                }

                auto& triangle_buffer = dynamic_cast<frame::vulkan::Buffer&>(
                    level_->GetBufferFromId(triangle_buffer_id));
                triangle_buffer.Copy(triangles);
                if (use_incremental_hardware_rt_updates)
                {
                    updated_source_triangle_buffer_ids.push_back(
                        triangle_buffer_id);
                }
                else if (
                    buffer_resources_->UpdateStorageBuffer(
                        level_->GetNameFromId(triangle_buffer_id),
                        triangle_buffer.GetRawData()))
                {
                    ++updated_buffer_count;
                    updated_source_triangle_buffer_ids.push_back(
                        triangle_buffer_id);
                }
            }
        }

        const auto bvh_buffer_id = skinned_mesh->GetBvhBufferId();
        if (bvh_buffer_id && skinned_mesh->HasActiveRaytraceBvhCallback())
        {
            auto bvh_nodes = skinned_mesh->EvaluateRaytraceBvh(skinning_time);
            if (!bvh_nodes.empty())
            {
                auto& bvh_buffer = dynamic_cast<frame::vulkan::Buffer&>(
                    level_->GetBufferFromId(bvh_buffer_id));
                bvh_buffer.Copy(
                    bvh_nodes.size() * sizeof(frame::BVHNode),
                    bvh_nodes.data());
                if (buffer_resources_->UpdateStorageBuffer(
                        level_->GetNameFromId(bvh_buffer_id),
                        bvh_buffer.GetRawData()))
                {
                    ++updated_buffer_count;
                }
            }
        }
    }
    bool updated_aggregate_scene = false;
    bool updated_hardware_transforms = false;
    if (HasRaytracingSourceMeshes(*level_))
    {
        bool used_incremental_source_updates = false;
        if (use_hardware_raytracing_ && hardware_raytracing_uses_source_instances_ &&
            !updated_source_triangle_buffer_ids.empty())
        {
            const bool updated_aggregate_ranges =
                UpdateHardwareRaytracingAggregateSceneBuffers(
                    updated_source_triangle_buffer_ids);
            const bool updated_dynamic_geometry =
                UpdateHardwareRaytracingDynamicGeometry(
                    updated_source_triangle_buffer_ids);
            if (updated_aggregate_ranges && updated_dynamic_geometry)
            {
                used_incremental_source_updates = true;
                updated_aggregate_scene = true;
                last_raytrace_scene_state_hash_ = BuildRaytracingSourceStateHash(
                    *level_,
                    static_cast<double>(elapsed_time_seconds_),
                    false);
                has_raytrace_scene_state_hash_ = true;
            }
        }

        if (!used_incremental_source_updates)
        {
            updated_aggregate_scene = UpdateAggregateRaytracingSceneBuffers(
                !use_hardware_raytracing_);
            if (use_hardware_raytracing_)
            {
                if (updated_aggregate_scene)
                {
                    if (!UpdateHardwareRaytracingDynamicGeometry())
                    {
                        UpdateHardwareRaytracingScene();
                    }
                }
                else
                {
                    updated_hardware_transforms =
                        UpdateHardwareRaytracingTransforms();
                }
            }
        }
    }
    if (updated_buffer_count > 0 || updated_aggregate_scene ||
        updated_hardware_transforms)
    {
        // Re-arm transfer->compute visibility barrier after dynamic SSBO writes.
        storage_buffers_ready_ = false;
    }
    static bool logged_once = false;
    if (!logged_once && updated_buffer_count > 0)
    {
        logger_->info(
            "Updated {} skinned Vulkan raytracing buffer(s) this frame.",
            updated_buffer_count);
        logged_once = true;
    }
}

bool Device::UpdateAggregateRaytracingSceneBuffers(bool build_software_bvh)
{
    if (!level_ || !buffer_resources_ || !active_program_info_)
    {
        return false;
    }
    const bool has_scene_triangle_buffers =
        active_program_info_->buffer_ids_by_inner.contains(
            "TriangleBufferTransmissive") ||
        active_program_info_->buffer_ids_by_inner.contains(
            "TriangleBufferOpaque");
    if (!has_scene_triangle_buffers)
    {
        return false;
    }
    const bool use_world_space_triangles =
        build_software_bvh || RaytraceSceneRequiresWorldSpaceBuffers(*level_);
    const std::size_t scene_state_hash =
        BuildRaytracingSourceStateHash(
            *level_,
            static_cast<double>(elapsed_time_seconds_),
            use_world_space_triangles);
    if (has_raytrace_scene_state_hash_ &&
        last_raytrace_scene_state_hash_ == scene_state_hash)
    {
        return false;
    }

    auto update_buffer = [&](const char* inner_name,
                             const std::vector<std::uint8_t>& bytes) {
        const auto it = active_program_info_->buffer_ids_by_inner.find(inner_name);
        if (it == active_program_info_->buffer_ids_by_inner.end())
        {
            return false;
        }

        auto* buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level_->GetBufferFromId(it->second));
        if (!buffer)
        {
            return false;
        }
        if (buffer->GetRawData() == bytes)
        {
            return false;
        }

        buffer->Copy(bytes);
        if (bytes.empty())
        {
            return true;
        }
        if (!buffer_resources_->UpdateStorageBuffer(
                level_->GetNameFromId(it->second),
                buffer->GetRawData()))
        {
            logger_->warn(
                "Failed to update animated Vulkan raytracing buffer '{}'.",
                inner_name);
        }
        return true;
    };

    const auto transmissive_triangles =
        BuildAggregateTriangleBytes(
            *level_,
            true,
            static_cast<double>(elapsed_time_seconds_),
            use_world_space_triangles);
    const auto opaque_triangles =
        BuildAggregateTriangleBytes(
            *level_,
            false,
            static_cast<double>(elapsed_time_seconds_),
            use_world_space_triangles);

    bool updated = false;
    updated |= update_buffer(
        "TriangleBufferTransmissive", transmissive_triangles);
    updated |= update_buffer("TriangleBufferOpaque", opaque_triangles);

    if (build_software_bvh)
    {
        updated |= update_buffer(
            "BvhBufferTransmissive",
            BuildAggregateBvhBytes(transmissive_triangles));
        updated |= update_buffer(
            "BvhBufferOpaque",
            BuildAggregateBvhBytes(opaque_triangles));
    }

    last_raytrace_scene_state_hash_ = scene_state_hash;
    has_raytrace_scene_state_hash_ = true;
    return updated;
}

bool Device::UpdateHardwareRaytracingInstanceStorageBuffer()
{
    if (!level_ || !active_program_info_)
    {
        return false;
    }

    const auto it =
        active_program_info_->buffer_ids_by_inner.find("RaytraceInstanceBuffer");
    if (it == active_program_info_->buffer_ids_by_inner.end())
    {
        return false;
    }

    auto* instance_buffer = dynamic_cast<frame::vulkan::Buffer*>(
        &level_->GetBufferFromId(it->second));
    if (!instance_buffer)
    {
        return false;
    }

    std::vector<HardwareRaytraceInstanceStorageData> instances = {};
    instances.reserve(hardware_raytracing_geometries_.size());
    for (const auto& geometry : hardware_raytracing_geometries_)
    {
        HardwareRaytraceInstanceStorageData instance = {};
        if (geometry.source_node_id != NullId)
        {
            auto* node = dynamic_cast<NodeMesh*>(
                &level_->GetSceneNodeFromId(geometry.source_node_id));
            if (node)
            {
                instance.object_to_world = node->GetLocalModel(
                    static_cast<double>(elapsed_time_seconds_));
                instance.world_to_object =
                    InverseOrIdentity(instance.object_to_world);
            }
        }
        instance.metadata.x = geometry.triangle_offset;
        instance.metadata.y = geometry.material_id;
        instance.metadata.z = 0u;
        instances.push_back(instance);
    }

    std::vector<std::uint8_t> bytes(
        instances.size() * sizeof(HardwareRaytraceInstanceStorageData));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), instances.data(), bytes.size());
    }

    const auto previous_bytes = instance_buffer->GetRawData();
    if (previous_bytes == bytes)
    {
        return false;
    }

    instance_buffer->Copy(bytes);

    if (buffer_resources_ && !bytes.empty())
    {
        const auto buffer_name = level_->GetNameFromId(it->second);
        if (previous_bytes.size() == bytes.size())
        {
            if (!buffer_resources_->UpdateStorageBuffer(buffer_name, bytes))
            {
                logger_->warn(
                    "Failed to upload Vulkan raytrace instance buffer '{}'.",
                    buffer_name);
            }
        }
        else if (!previous_bytes.empty())
        {
            logger_->warn(
                "Vulkan raytrace instance buffer '{}' changed size from {} to {}; GPU upload requires a resource rebuild.",
                buffer_name,
                previous_bytes.size(),
                bytes.size());
        }
    }

    return true;
}

bool Device::UpdateHardwareRaytracingAggregateSceneBuffers(
    const std::vector<EntityId>& updated_source_triangle_buffer_ids)
{
    if (!level_ || !buffer_resources_ || !active_program_info_ ||
        !hardware_raytracing_uses_source_instances_ ||
        updated_source_triangle_buffer_ids.empty())
    {
        return false;
    }

    const auto was_updated = [&](EntityId buffer_id) {
        return std::find(
                   updated_source_triangle_buffer_ids.begin(),
                   updated_source_triangle_buffer_ids.end(),
                   buffer_id) != updated_source_triangle_buffer_ids.end();
    };

    bool updated = false;
    for (const auto& geometry : hardware_raytracing_geometries_)
    {
        if (!was_updated(geometry.source_buffer_id) ||
            geometry.source_node_id == NullId ||
            geometry.source_material_id == NullId)
        {
            continue;
        }

        auto source_geometry = BuildRaytracingSourceGeometryDataForSource(
            *level_,
            geometry.source_node_id,
            geometry.source_material_id,
            static_cast<double>(elapsed_time_seconds_),
            false,
            geometry.triangle_offset);
        if (!source_geometry ||
            source_geometry->triangle_buffer_id != geometry.source_buffer_id ||
            source_geometry->material_id != geometry.material_id)
        {
            return false;
        }

        const char* inner_name =
            geometry.material_id == 0u
                ? "TriangleBufferTransmissive"
                : "TriangleBufferOpaque";
        const auto it = active_program_info_->buffer_ids_by_inner.find(inner_name);
        if (it == active_program_info_->buffer_ids_by_inner.end())
        {
            return false;
        }

        auto* aggregate_buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level_->GetBufferFromId(it->second));
        if (!aggregate_buffer)
        {
            return false;
        }

        const std::size_t byte_offset = static_cast<std::size_t>(
            geometry.triangle_offset) *
            (kRaytraceTriangleVertexStrideBytes * 3u);
        if (byte_offset > aggregate_buffer->GetRawData().size() ||
            source_geometry->triangle_bytes.size() >
                aggregate_buffer->GetRawData().size() - byte_offset)
        {
            return false;
        }

        if (!aggregate_buffer->CopyRange(
                byte_offset,
                source_geometry->triangle_bytes))
        {
            continue;
        }

        if (!buffer_resources_->UpdateStorageBufferRange(
                level_->GetNameFromId(it->second),
                source_geometry->triangle_bytes,
                byte_offset))
        {
            logger_->warn(
                "Failed to update Vulkan raytrace aggregate buffer '{}' range [{}..{}).",
                inner_name,
                byte_offset,
                byte_offset + source_geometry->triangle_bytes.size());
        }
        updated = true;
    }

    return updated;
}

bool Device::UpdateHardwareRaytracingDynamicGeometry()
{
    std::vector<EntityId> updated_source_triangle_buffer_ids = {};
    updated_source_triangle_buffer_ids.reserve(hardware_raytracing_geometries_.size());
    for (const auto& geometry : hardware_raytracing_geometries_)
    {
        if (geometry.source_buffer_id != NullId)
        {
            updated_source_triangle_buffer_ids.push_back(geometry.source_buffer_id);
        }
    }
    return UpdateHardwareRaytracingDynamicGeometry(
        updated_source_triangle_buffer_ids);
}

bool Device::UpdateHardwareRaytracingDynamicGeometry(
    const std::vector<EntityId>& updated_source_triangle_buffer_ids)
{
    if (!use_hardware_raytracing_ || !vk_unique_device_ || !level_ ||
        !gpu_memory_manager_ || !command_queue_ ||
        hardware_raytracing_geometries_.empty() || !hardware_raytracing_tlas_ ||
        !hardware_raytracing_uses_source_instances_ ||
        updated_source_triangle_buffer_ids.empty())
    {
        return false;
    }

    const auto build_scratch_address =
        [&](vk::DeviceSize size,
            vk::UniqueBuffer& scratch_buffer,
            vk::UniqueDeviceMemory& scratch_memory) {
            scratch_buffer = gpu_memory_manager_->CreateBuffer(
                size,
                vk::BufferUsageFlagBits::eStorageBuffer |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                scratch_memory,
                vk::MemoryAllocateFlagBits::eDeviceAddress);
            return vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*scratch_buffer));
        };

    const auto was_updated = [&](EntityId buffer_id) {
        return std::find(
                   updated_source_triangle_buffer_ids.begin(),
                   updated_source_triangle_buffer_ids.end(),
                   buffer_id) != updated_source_triangle_buffer_ids.end();
    };

    struct PendingBlasUpdate
    {
        HardwareRaytracingGeometry* geometry = nullptr;
        vk::UniqueBuffer staging_buffer;
        vk::UniqueDeviceMemory staging_memory;
        vk::UniqueBuffer scratch_buffer;
        vk::UniqueDeviceMemory scratch_memory;
        vk::AccelerationStructureGeometryTrianglesDataKHR triangles = {};
        vk::AccelerationStructureGeometryDataKHR geometry_data = {};
        vk::AccelerationStructureGeometryKHR as_geometry = {};
        vk::AccelerationStructureBuildGeometryInfoKHR build_info = {};
        vk::AccelerationStructureBuildRangeInfoKHR range_info = {};
        vk::DeviceSize upload_size = 0;
    };
    std::vector<PendingBlasUpdate> pending_updates = {};

    for (auto& geometry : hardware_raytracing_geometries_)
    {
        if (!was_updated(geometry.source_buffer_id) ||
            geometry.source_node_id == NullId ||
            geometry.source_material_id == NullId)
        {
            continue;
        }

        auto source_geometry = BuildRaytracingSourceGeometryDataForSource(
            *level_,
            geometry.source_node_id,
            geometry.source_material_id,
            static_cast<double>(elapsed_time_seconds_),
            false,
            geometry.triangle_offset);
        if (!source_geometry)
        {
            return false;
        }

        if (!geometry.blas || !geometry.vertex_buffer || !geometry.index_buffer ||
            geometry.source_node_id != source_geometry->source_node_id ||
            geometry.source_buffer_id != source_geometry->triangle_buffer_id ||
            geometry.material_id != source_geometry->material_id ||
            geometry.triangle_offset != source_geometry->triangle_offset)
        {
            return false;
        }

        const auto& triangle_bytes = source_geometry->triangle_bytes;
        if (triangle_bytes.size() !=
            static_cast<std::size_t>(geometry.vertex_buffer_size))
        {
            return false;
        }

        const auto vertex_count = static_cast<std::uint32_t>(
            triangle_bytes.size() / kRaytraceTriangleVertexStrideBytes);
        const auto primitive_count = vertex_count / 3u;
        if (vertex_count != geometry.vertex_count ||
            primitive_count != geometry.triangle_count)
        {
            return false;
        }

        PendingBlasUpdate pending = {};
        pending.geometry = &geometry;
        pending.upload_size = static_cast<vk::DeviceSize>(triangle_bytes.size());

        pending.staging_buffer = gpu_memory_manager_->CreateBuffer(
            pending.upload_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            pending.staging_memory);
        void* mapped = vk_unique_device_->mapMemory(
            *pending.staging_memory,
            0,
            pending.upload_size);
        std::memcpy(mapped, triangle_bytes.data(), triangle_bytes.size());
        vk_unique_device_->unmapMemory(*pending.staging_memory);

        const auto vertex_address = vk_unique_device_->getBufferAddress(
            vk::BufferDeviceAddressInfo(*geometry.vertex_buffer));
        const auto index_address = vk_unique_device_->getBufferAddress(
            vk::BufferDeviceAddressInfo(*geometry.index_buffer));
        pending.triangles = vk::AccelerationStructureGeometryTrianglesDataKHR(
            vk::Format::eR32G32B32Sfloat,
            vk::DeviceOrHostAddressConstKHR(vertex_address),
            kRaytraceTriangleVertexStrideBytes,
            geometry.vertex_count,
            vk::IndexType::eUint32,
            vk::DeviceOrHostAddressConstKHR(index_address));
        pending.geometry_data.setTriangles(pending.triangles);
        pending.as_geometry = vk::AccelerationStructureGeometryKHR(
            vk::GeometryTypeKHR::eTriangles);
        pending.as_geometry.setGeometry(pending.geometry_data);
        pending.as_geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        pending.build_info = vk::AccelerationStructureBuildGeometryInfoKHR(
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            GetHardwareRaytracingBuildFlags(),
            vk::BuildAccelerationStructureModeKHR::eUpdate,
            {},
            {},
            pending.as_geometry);
        pending.build_info.setSrcAccelerationStructure(*geometry.blas);
        pending.build_info.setDstAccelerationStructure(*geometry.blas);

        const auto size_info =
            vk_unique_device_->getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                pending.build_info,
                geometry.triangle_count);
        const auto scratch_address = build_scratch_address(
            std::max(
                size_info.buildScratchSize,
                size_info.updateScratchSize),
            pending.scratch_buffer,
            pending.scratch_memory);
        pending.build_info.setScratchData(
            vk::DeviceOrHostAddressKHR(scratch_address));
        pending.range_info = vk::AccelerationStructureBuildRangeInfoKHR(
            geometry.triangle_count,
            0,
            0,
            0);
        pending_updates.push_back(std::move(pending));
    }

    if (pending_updates.empty())
    {
        return false;
    }

    command_queue_->SubmitOneTime(
        [&](vk::CommandBuffer command_buffer) {
            std::vector<vk::BufferMemoryBarrier> copy_barriers = {};
            copy_barriers.reserve(pending_updates.size());
            for (const auto& pending : pending_updates)
            {
                command_buffer.copyBuffer(
                    *pending.staging_buffer,
                    *pending.geometry->vertex_buffer,
                    vk::BufferCopy(0, 0, pending.upload_size));
                copy_barriers.emplace_back(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eAccelerationStructureReadKHR,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    *pending.geometry->vertex_buffer,
                    0,
                    pending.upload_size);
            }
            if (!copy_barriers.empty())
            {
                command_buffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                    {},
                    nullptr,
                    copy_barriers,
                    nullptr);
            }
            for (const auto& pending : pending_updates)
            {
                const vk::AccelerationStructureBuildRangeInfoKHR* range_infos[] = {
                    &pending.range_info};
                command_buffer.buildAccelerationStructuresKHR(
                    pending.build_info,
                    range_infos);
            }
        });

    return UpdateHardwareRaytracingTransforms(true);
}

void Device::UpdateHardwareRaytracingScene()
{
    CreateHardwareRaytracingScene();
    UpdateHardwareRaytracingDescriptor();
}

bool Device::UpdateHardwareRaytracingTransforms(bool force_tlas_update)
{
    if (!use_hardware_raytracing_ || !vk_unique_device_ || !level_ ||
        hardware_raytracing_geometries_.empty() || !hardware_raytracing_tlas_ ||
        !hardware_raytracing_instance_buffer_ || !hardware_raytracing_instance_memory_)
    {
        return false;
    }
    if (RaytraceSceneRequiresWorldSpaceBuffers(*level_))
    {
        return false;
    }

    std::vector<vk::AccelerationStructureInstanceKHR> instances = {};
    instances.reserve(hardware_raytracing_geometries_.size());
    for (const auto& geometry : hardware_raytracing_geometries_)
    {
        vk::AccelerationStructureInstanceKHR instance{};
        if (geometry.source_node_id != NullId)
        {
            auto* node = dynamic_cast<NodeMesh*>(
                &level_->GetSceneNodeFromId(geometry.source_node_id));
            if (node)
            {
                instance.transform = MakeAccelerationStructureTransform(
                    node->GetLocalModel(static_cast<double>(elapsed_time_seconds_)));
            }
            else
            {
                instance.transform = MakeIdentityTransform();
            }
        }
        else
        {
            instance.transform = MakeIdentityTransform();
        }
        instance.instanceCustomIndex = geometry.instance_custom_index;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = static_cast<VkGeometryInstanceFlagsKHR>(
            vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instance.accelerationStructureReference = geometry.blas_address;
        instances.push_back(instance);
    }

    std::vector<std::uint8_t> bytes(
        instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    if (!bytes.empty())
    {
        std::memcpy(bytes.data(), instances.data(), bytes.size());
    }

    const bool storage_buffer_changed =
        UpdateHardwareRaytracingInstanceStorageBuffer();
    if (hardware_raytracing_instance_bytes_ == bytes)
    {
        if (!force_tlas_update)
        {
            return storage_buffer_changed;
        }
        RebuildHardwareRaytracingTlas();
        return true;
    }

    if (sync_resources_ && sync_resources_->IsCreated())
    {
        std::vector<VkFence> fences = {};
        fences.reserve(sync_resources_->GetFrameCount());
        for (std::size_t i = 0; i < sync_resources_->GetFrameCount(); ++i)
        {
            fences.push_back(
                static_cast<VkFence>(sync_resources_->GetInFlightFence(i)));
        }
        if (!fences.empty())
        {
            const VkResult wait_result = vkWaitForFences(
                static_cast<VkDevice>(*vk_unique_device_),
                static_cast<std::uint32_t>(fences.size()),
                fences.data(),
                VK_TRUE,
                std::numeric_limits<std::uint64_t>::max());
            if (wait_result != VK_SUCCESS)
            {
                logger_->error(
                    "vkWaitForFences failed before updating hardware raytracing transforms: {}",
                    vk::to_string(static_cast<vk::Result>(wait_result)));
                if (wait_result == VK_ERROR_DEVICE_LOST)
                {
                    device_lost_ = true;
                }
                return storage_buffer_changed;
            }
        }
    }

    void* mapped_instances = vk_unique_device_->mapMemory(
        *hardware_raytracing_instance_memory_,
        0,
        static_cast<vk::DeviceSize>(bytes.size()));
    if (!bytes.empty())
    {
        std::memcpy(mapped_instances, bytes.data(), bytes.size());
    }
    vk_unique_device_->unmapMemory(*hardware_raytracing_instance_memory_);
    hardware_raytracing_instance_bytes_ = bytes;
    RebuildHardwareRaytracingTlas();
    return true;
}

void Device::RebuildHardwareRaytracingTlas()
{
    if (!use_hardware_raytracing_ || !vk_unique_device_ || !gpu_memory_manager_ ||
        !command_queue_ || !hardware_raytracing_tlas_ ||
        !hardware_raytracing_instance_buffer_)
    {
        return;
    }

    const std::uint32_t instance_count =
        static_cast<std::uint32_t>(hardware_raytracing_geometries_.size());
    if (instance_count == 0)
    {
        return;
    }

    const auto build_scratch_address =
        [&](vk::DeviceSize size,
            vk::UniqueBuffer& scratch_buffer,
            vk::UniqueDeviceMemory& scratch_memory) {
            scratch_buffer = gpu_memory_manager_->CreateBuffer(
                size,
                vk::BufferUsageFlagBits::eStorageBuffer |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                scratch_memory,
                vk::MemoryAllocateFlagBits::eDeviceAddress);
            return vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*scratch_buffer));
        };

    const auto instance_address = vk_unique_device_->getBufferAddress(
        vk::BufferDeviceAddressInfo(*hardware_raytracing_instance_buffer_));
    vk::AccelerationStructureGeometryInstancesDataKHR instances_data(
        VK_FALSE,
        vk::DeviceOrHostAddressConstKHR(instance_address));
    vk::AccelerationStructureGeometryDataKHR tlas_geometry_data;
    tlas_geometry_data.setInstances(instances_data);
    vk::AccelerationStructureGeometryKHR tlas_geometry(
        vk::GeometryTypeKHR::eInstances);
    tlas_geometry.setGeometry(tlas_geometry_data);

    vk::AccelerationStructureBuildGeometryInfoKHR tlas_build_info(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        GetHardwareRaytracingBuildFlags(),
        vk::BuildAccelerationStructureModeKHR::eUpdate,
        {},
        {},
        tlas_geometry);
    tlas_build_info.setSrcAccelerationStructure(*hardware_raytracing_tlas_);
    const auto tlas_size = vk_unique_device_->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        tlas_build_info,
        instance_count);
    vk::UniqueBuffer tlas_scratch_buffer;
    vk::UniqueDeviceMemory tlas_scratch_memory;
    const auto tlas_scratch_address = build_scratch_address(
        std::max(tlas_size.buildScratchSize, tlas_size.updateScratchSize),
        tlas_scratch_buffer,
        tlas_scratch_memory);
    tlas_build_info.setDstAccelerationStructure(*hardware_raytracing_tlas_);
    tlas_build_info.setScratchData(
        vk::DeviceOrHostAddressKHR(tlas_scratch_address));
    vk::AccelerationStructureBuildRangeInfoKHR tlas_range_info(
        instance_count,
        0,
        0,
        0);
    const vk::AccelerationStructureBuildRangeInfoKHR* tlas_ranges[] = {
        &tlas_range_info};
    command_queue_->SubmitOneTime(
        [&](vk::CommandBuffer command_buffer) {
            command_buffer.buildAccelerationStructuresKHR(
                tlas_build_info,
                tlas_ranges);
        });
}

void Device::UpdateHardwareRaytracingDescriptor()
{
    if (!vk_unique_device_ || !descriptor_set_ || !use_hardware_raytracing_ ||
        !hardware_raytracing_tlas_)
    {
        return;
    }

    std::array<vk::AccelerationStructureKHR, 1> acceleration_handles = {
        hardware_raytracing_tlas_.get()};
    vk::WriteDescriptorSetAccelerationStructureKHR acceleration_write(
        static_cast<std::uint32_t>(acceleration_handles.size()),
        acceleration_handles.data());
    vk::WriteDescriptorSet descriptor_write(
        descriptor_set_,
        kHardwareRaytracingAsBinding,
        0,
        static_cast<std::uint32_t>(acceleration_handles.size()),
        vk::DescriptorType::eAccelerationStructureKHR);
    descriptor_write.setPNext(&acceleration_write);
    const std::array<vk::WriteDescriptorSet, 1> descriptor_writes = {
        descriptor_write};
    vk_unique_device_->updateDescriptorSets(
        static_cast<std::uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(),
        0,
        nullptr);
}

std::optional<vk::DescriptorImageInfo> Device::GetComputeOutputDescriptorInfo() const
{
    if (!output_image_resources_)
    {
        return std::nullopt;
    }
    return output_image_resources_->GetComputeOutputDescriptorInfo();
}

std::optional<vk::DescriptorImageInfo> Device::GetSwapchainPreviewDescriptorInfo() const
{
    if (!output_image_resources_)
    {
        return std::nullopt;
    }
    return output_image_resources_->GetSwapchainPreviewDescriptorInfo();
}

void Device::Display(double dt)
{
    if (renderer_)
    {
        renderer_->Display(dt);
    }
}

void Device::LogRuntimeConfiguration() const
{
    const bool validation_enabled = absl::GetFlag(FLAGS_vk_validation);
    const bool has_active_program = active_program_info_.has_value();
    const bool has_compute_shader =
        has_active_program && !active_program_info_->compute_shader.empty();
    const bool has_raytracing_stage_mapping =
        has_active_program &&
        !active_program_info_->raygen_shader.empty() &&
        !active_program_info_->miss_shader.empty() &&
        !active_program_info_->closesthit_shader.empty();
    const bool raytracing_pipeline_ready =
        pipeline_resources_ && pipeline_resources_->HasRaytracingPipeline();
    const bool compute_pipeline_ready =
        pipeline_resources_ && pipeline_resources_->HasComputePipeline();
    const bool hardware_scene_ready =
        static_cast<bool>(hardware_raytracing_tlas_);

    std::string active_path = "graphics";
    if (raytracing_pipeline_ready && hardware_scene_ready)
    {
        active_path = "hardware_rt";
    }
    else if (compute_pipeline_ready)
    {
        active_path = "compute_fallback";
    }
    else if (raytracing_pipeline_ready)
    {
        active_path = "raytracing_pipeline_without_scene";
    }
    else if (use_raytracing_pipeline_)
    {
        active_path = "hardware_rt_requested_pipeline_missing";
    }
    else if (use_compute_raytracing_)
    {
        active_path = "compute_requested_pipeline_missing";
    }

    std::string material_name = "<none>";
    if (level_ && has_active_program && active_program_info_->material_id != NullId)
    {
        material_name = level_->GetNameFromId(active_program_info_->material_id);
    }

    logger_->info(
        "Vulkan runtime summary: validation={}, program='{}', material='{}', path={}, hw_rt_supported={}, hw_rt_requested={}, hw_rt_scene_ready={}, rt_pipeline_ready={}, compute_pipeline_ready={}, compute_shader_mapped={}, rt_stage_mapping={}.",
        BoolToString(validation_enabled),
        has_active_program ? active_program_info_->program_name.c_str() : "<none>",
        material_name,
        active_path,
        BoolToString(hardware_raytracing_supported_),
        BoolToString(use_hardware_raytracing_),
        BoolToString(hardware_scene_ready),
        BoolToString(raytracing_pipeline_ready),
        BoolToString(compute_pipeline_ready),
        BoolToString(has_compute_shader),
        BoolToString(has_raytracing_stage_mapping));
}


void Device::Shutdown()
{
    Cleanup();
    vk_unique_device_.reset();
}

void Device::ScreenShot(const std::string& file) const
{
    logger_->warn("Vulkan screenshot not implemented (requested: {})", file);
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(
    std::vector<float>&& /*vector*/)
{
    throw std::runtime_error("Vulkan point buffer support not implemented yet.");
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& /*vector*/)
{
    throw std::runtime_error("Vulkan index buffer support not implemented yet.");
}

std::unique_ptr<frame::MeshInterface> Device::CreateMesh(
    const MeshParameter& /*mesh_parameter*/)
{
    throw std::runtime_error("Vulkan mesh support not implemented yet.");
}

void Device::RecreateSwapchain()
{
    if (size_.x == 0 || size_.y == 0)
    {
        return;
    }
    if (!swapchain_resources_ || !command_resources_)
    {
        return;
    }
    if (vk_unique_device_)
    {
        vk_unique_device_->waitIdle();
    }

    DestroyRaytracingPipeline();
    DestroyComputePipeline();
    DestroyDescriptorResources();
    DestroyHardwareRaytracingScene();
    DestroyGraphicsPipeline();
    DestroySwapchainPreviewImage();
    swapchain_resources_->Destroy();
    command_resources_->FreeBuffers();

    swapchain_resources_->Create(size_);
    command_resources_->AllocateBuffers(
        static_cast<std::uint32_t>(kMaxFramesInFlight));
    if (sync_resources_)
    {
        sync_resources_->Destroy();
        sync_resources_->SetSwapchainImageCount(
            swapchain_resources_->GetImages().size());
        sync_resources_->Create();
    }
    if (renderer_)
    {
        renderer_->Reset();
    }
    CreateSwapchainPreviewImage();

    if (use_hardware_raytracing_)
    {
        CreateHardwareRaytracingScene();
    }
    CreateDescriptorResources();
    CreateGraphicsPipeline();
    if (use_raytracing_pipeline_)
    {
        CreateRaytracingPipeline();
    }
    else if (use_compute_raytracing_)
    {
        CreateComputePipeline();
    }
}

SceneState Device::BuildFrameSceneState(vk::Extent2D extent) const
{
    const bool has_raytrace_source_meshes =
        level_ && HasRaytracingSourceMeshes(*level_);
    const bool use_shared_transform_hardware_scene =
        has_raytrace_source_meshes &&
        use_hardware_raytracing_ &&
        CanUseSharedTransformHardwareRaytraceScene(
            *level_,
            static_cast<double>(elapsed_time_seconds_));
    const bool use_world_space_raytrace_scene =
        has_raytrace_source_meshes &&
        (use_compute_raytracing_ || use_raytracing_pipeline_) &&
        !use_shared_transform_hardware_scene;
    std::string preferred_scene_root;
    if (!use_world_space_raytrace_scene &&
        level_ &&
        active_program_info_ &&
        active_program_info_->program_id != NullId)
    {
        preferred_scene_root =
            level_->GetProgramFromId(active_program_info_->program_id)
                .GetTemporarySceneRoot();
    }

    if (!level_)
    {
        return SceneState{};
    }
    return BuildSceneState(
        *level_,
        frame::Logger::GetInstance(),
        {extent.width, extent.height},
        elapsed_time_seconds_,
        active_program_info_ ? active_program_info_->material_id : NullId,
        !use_compute_raytracing_,
        preferred_scene_root,
        use_world_space_raytrace_scene);
}

void Device::CreateGraphicsPipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->CreateGraphicsPipeline();
    }
}

void Device::DestroyGraphicsPipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->DestroyGraphicsPipeline();
    }
}

void Device::CreateComputePipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->CreateComputePipeline();
    }
}

void Device::DestroyComputePipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->DestroyComputePipeline();
    }
}

void Device::CreateRaytracingPipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->CreateRaytracingPipeline();
    }
}

void Device::DestroyRaytracingPipeline()
{
    if (pipeline_resources_)
    {
        pipeline_resources_->DestroyRaytracingPipeline();
    }
}

void Device::CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    if (command_queue_)
    {
        command_queue_->CopyBuffer(src, dst, size);
    }
}

void Device::TransitionImageLayout(
    vk::Image image,
    vk::Format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    std::uint32_t layer_count)
{
    if (command_queue_)
    {
        command_queue_->TransitionImageLayout(
            image, {}, old_layout, new_layout, layer_count);
    }
}

void Device::CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t layer_count,
    std::size_t layer_stride)
{
    if (command_queue_)
    {
        command_queue_->CopyBufferToImage(
            buffer, image, width, height, layer_count, layer_stride);
    }
}

void Device::DestroyTextureResources()
{
    if (texture_resources_)
    {
        texture_resources_->Destroy();
    }
}

void Device::DestroyDescriptorResources()
{
    descriptor_set_ = VK_NULL_HANDLE;
    descriptor_pool_.reset();
    descriptor_set_layout_.reset();
    if (buffer_resources_)
    {
        buffer_resources_->Clear();
    }
    storage_buffers_ready_ = false;
    DestroyComputeOutputImage();
}

void Device::CreateHardwareRaytracingScene()
{
    DestroyHardwareRaytracingScene();

    if (!use_hardware_raytracing_ || !vk_unique_device_ || !level_ ||
        !gpu_memory_manager_ || !command_queue_)
    {
        return;
    }

    const auto create_gpu_input_buffer =
        [&](const std::vector<std::uint8_t>& bytes,
            vk::BufferUsageFlags usage,
            vk::UniqueBuffer& out_buffer,
            vk::UniqueDeviceMemory& out_memory) {
            if (bytes.empty())
            {
                throw std::runtime_error(
                    "Cannot create Vulkan hardware raytracing input buffer from empty data.");
            }

            vk::UniqueDeviceMemory staging_memory;
            auto staging_buffer = gpu_memory_manager_->CreateBuffer(
                static_cast<vk::DeviceSize>(bytes.size()),
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent,
                staging_memory);

            void* mapped =
                vk_unique_device_->mapMemory(
                    *staging_memory,
                    0,
                    static_cast<vk::DeviceSize>(bytes.size()));
            std::memcpy(mapped, bytes.data(), bytes.size());
            vk_unique_device_->unmapMemory(*staging_memory);

            out_buffer = gpu_memory_manager_->CreateBuffer(
                static_cast<vk::DeviceSize>(bytes.size()),
                usage |
                    vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                out_memory,
                vk::MemoryAllocateFlagBits::eDeviceAddress);
            command_queue_->CopyBuffer(
                *staging_buffer,
                *out_buffer,
                static_cast<vk::DeviceSize>(bytes.size()));
        };

    const auto create_acceleration_structure =
        [&](vk::AccelerationStructureTypeKHR type,
            vk::DeviceSize size,
            vk::UniqueAccelerationStructureKHR& out_as,
            vk::UniqueBuffer& out_buffer,
            vk::UniqueDeviceMemory& out_memory) {
            out_buffer = gpu_memory_manager_->CreateBuffer(
                size,
                vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                out_memory,
                vk::MemoryAllocateFlagBits::eDeviceAddress);
            vk::AccelerationStructureCreateInfoKHR as_info(
                {},
                *out_buffer,
                0,
                size,
                type);
            out_as = vk_unique_device_->createAccelerationStructureKHRUnique(as_info);
        };

    const auto build_scratch_buffer =
        [&](vk::DeviceSize size,
            vk::UniqueBuffer& scratch_buffer,
            vk::UniqueDeviceMemory& scratch_memory) {
            scratch_buffer = gpu_memory_manager_->CreateBuffer(
                size,
                vk::BufferUsageFlagBits::eStorageBuffer |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                scratch_memory,
                vk::MemoryAllocateFlagBits::eDeviceAddress);
            return vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*scratch_buffer));
        };

    std::vector<vk::AccelerationStructureInstanceKHR> instances = {};
    const bool use_source_instances =
        !RaytraceSceneRequiresWorldSpaceBuffers(*level_);
    hardware_raytracing_uses_source_instances_ = use_source_instances;
    const auto source_geometries = use_source_instances
        ? BuildRaytracingSourceGeometryData(
              *level_,
              static_cast<double>(elapsed_time_seconds_),
              false)
        : std::vector<RaytracingSourceGeometryData>{};
    instances.reserve(
        use_source_instances ? source_geometries.size() : static_cast<std::size_t>(2));

    std::uint32_t next_instance_custom_index = 0;
    const auto append_source_geometry =
        [&](const RaytracingSourceGeometryData& source_geometry) {
            const auto& triangle_bytes = source_geometry.triangle_bytes;
            const auto vertex_count = static_cast<std::uint32_t>(
                triangle_bytes.size() / kRaytraceTriangleVertexStrideBytes);
            const auto primitive_count = vertex_count / 3;
            if (primitive_count == 0)
            {
                return;
            }
            const auto index_bytes = BuildSequentialIndexBytes(vertex_count);

            HardwareRaytracingGeometry geometry = {};
            geometry.source_inner_name =
                level_->GetNameFromId(source_geometry.source_node_id);
            geometry.source_node_id = source_geometry.source_node_id;
            geometry.source_buffer_id = source_geometry.triangle_buffer_id;
            geometry.source_material_id = source_geometry.source_material_id;
            geometry.triangle_offset = source_geometry.triangle_offset;
            create_gpu_input_buffer(
                triangle_bytes,
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                geometry.vertex_buffer,
                geometry.vertex_memory);
            create_gpu_input_buffer(
                index_bytes,
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                geometry.index_buffer,
                geometry.index_memory);

            const auto vertex_address = vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*geometry.vertex_buffer));
            const auto index_address = vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*geometry.index_buffer));
            geometry.vertex_buffer_size =
                static_cast<vk::DeviceSize>(triangle_bytes.size());
            geometry.vertex_count = vertex_count;
            geometry.index_buffer_size =
                static_cast<vk::DeviceSize>(index_bytes.size());
            geometry.triangle_count = primitive_count;
            geometry.instance_custom_index = next_instance_custom_index++;
            geometry.material_id = source_geometry.material_id;

            vk::AccelerationStructureGeometryTrianglesDataKHR triangles(
                vk::Format::eR32G32B32Sfloat,
                vk::DeviceOrHostAddressConstKHR(vertex_address),
                kRaytraceTriangleVertexStrideBytes,
                vertex_count,
                vk::IndexType::eUint32,
                vk::DeviceOrHostAddressConstKHR(index_address));
            vk::AccelerationStructureGeometryDataKHR geometry_data;
            geometry_data.setTriangles(triangles);
            vk::AccelerationStructureGeometryKHR as_geometry(
                vk::GeometryTypeKHR::eTriangles);
            as_geometry.setGeometry(geometry_data);
            as_geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

            vk::AccelerationStructureBuildGeometryInfoKHR build_info(
                vk::AccelerationStructureTypeKHR::eBottomLevel,
                GetHardwareRaytracingBuildFlags(),
                vk::BuildAccelerationStructureModeKHR::eBuild,
                {},
                {},
                as_geometry);

            const auto size_info =
                vk_unique_device_->getAccelerationStructureBuildSizesKHR(
                    vk::AccelerationStructureBuildTypeKHR::eDevice,
                    build_info,
                    primitive_count);

            create_acceleration_structure(
                vk::AccelerationStructureTypeKHR::eBottomLevel,
                size_info.accelerationStructureSize,
                geometry.blas,
                geometry.blas_buffer,
                geometry.blas_memory);

            vk::UniqueBuffer scratch_buffer;
            vk::UniqueDeviceMemory scratch_memory;
            const auto scratch_address = build_scratch_buffer(
                size_info.buildScratchSize,
                scratch_buffer,
                scratch_memory);

            build_info.setDstAccelerationStructure(*geometry.blas);
            build_info.setScratchData(
                vk::DeviceOrHostAddressKHR(scratch_address));
            vk::AccelerationStructureBuildRangeInfoKHR range_info(
                primitive_count,
                0,
                0,
                0);
            const vk::AccelerationStructureBuildRangeInfoKHR* range_infos[] = {
                &range_info};
            command_queue_->SubmitOneTime(
                [&](vk::CommandBuffer command_buffer) {
                    command_buffer.buildAccelerationStructuresKHR(
                        build_info,
                        range_infos);
                });

            geometry.blas_address =
                vk_unique_device_->getAccelerationStructureAddressKHR(
                    vk::AccelerationStructureDeviceAddressInfoKHR(
                        *geometry.blas));

            vk::AccelerationStructureInstanceKHR instance{};
            auto* node = dynamic_cast<NodeMesh*>(
                &level_->GetSceneNodeFromId(source_geometry.source_node_id));
            if (node)
            {
                instance.transform = MakeAccelerationStructureTransform(
                    node->GetLocalModel(static_cast<double>(elapsed_time_seconds_)));
            }
            else
            {
                instance.transform = MakeIdentityTransform();
            }
            instance.instanceCustomIndex = geometry.instance_custom_index;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = static_cast<VkGeometryInstanceFlagsKHR>(
                vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instance.accelerationStructureReference = geometry.blas_address;
            instances.push_back(instance);
            hardware_raytracing_geometries_.push_back(std::move(geometry));
        };
    const auto append_geometry =
        [&](const char* inner_name,
            std::uint32_t material_id) {
            if (!active_program_info_)
            {
                return;
            }
            const auto it =
                active_program_info_->buffer_ids_by_inner.find(inner_name);
            if (it == active_program_info_->buffer_ids_by_inner.end())
            {
                return;
            }
            auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
                &level_->GetBufferFromId(it->second));
            if (!triangle_buffer)
            {
                return;
            }
            const auto& triangle_bytes = triangle_buffer->GetRawData();
            const auto vertex_count = static_cast<std::uint32_t>(
                triangle_bytes.size() / kRaytraceTriangleVertexStrideBytes);
            const auto primitive_count = vertex_count / 3;
            if (primitive_count == 0)
            {
                return;
            }
            const auto index_bytes = BuildSequentialIndexBytes(vertex_count);

            HardwareRaytracingGeometry geometry = {};
            geometry.source_inner_name = inner_name;
            geometry.source_buffer_id = it->second;
            create_gpu_input_buffer(
                triangle_bytes,
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                geometry.vertex_buffer,
                geometry.vertex_memory);
            create_gpu_input_buffer(
                index_bytes,
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                geometry.index_buffer,
                geometry.index_memory);

            const auto vertex_address = vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*geometry.vertex_buffer));
            const auto index_address = vk_unique_device_->getBufferAddress(
                vk::BufferDeviceAddressInfo(*geometry.index_buffer));
            geometry.vertex_buffer_size =
                static_cast<vk::DeviceSize>(triangle_bytes.size());
            geometry.vertex_count = vertex_count;
            geometry.index_buffer_size =
                static_cast<vk::DeviceSize>(index_bytes.size());
            geometry.triangle_count = primitive_count;
            geometry.instance_custom_index = next_instance_custom_index++;
            geometry.material_id = material_id;
            geometry.triangle_offset = 0;

            vk::AccelerationStructureGeometryTrianglesDataKHR triangles(
                vk::Format::eR32G32B32Sfloat,
                vk::DeviceOrHostAddressConstKHR(vertex_address),
                kRaytraceTriangleVertexStrideBytes,
                vertex_count,
                vk::IndexType::eUint32,
                vk::DeviceOrHostAddressConstKHR(index_address));
            vk::AccelerationStructureGeometryDataKHR geometry_data;
            geometry_data.setTriangles(triangles);
            vk::AccelerationStructureGeometryKHR as_geometry(
                vk::GeometryTypeKHR::eTriangles);
            as_geometry.setGeometry(geometry_data);
            as_geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

            vk::AccelerationStructureBuildGeometryInfoKHR build_info(
                vk::AccelerationStructureTypeKHR::eBottomLevel,
                GetHardwareRaytracingBuildFlags(),
                vk::BuildAccelerationStructureModeKHR::eBuild,
                {},
                {},
                as_geometry);

            const auto size_info =
                vk_unique_device_->getAccelerationStructureBuildSizesKHR(
                    vk::AccelerationStructureBuildTypeKHR::eDevice,
                    build_info,
                    primitive_count);

            create_acceleration_structure(
                vk::AccelerationStructureTypeKHR::eBottomLevel,
                size_info.accelerationStructureSize,
                geometry.blas,
                geometry.blas_buffer,
                geometry.blas_memory);

            vk::UniqueBuffer scratch_buffer;
            vk::UniqueDeviceMemory scratch_memory;
            const auto scratch_address = build_scratch_buffer(
                size_info.buildScratchSize,
                scratch_buffer,
                scratch_memory);

            build_info.setDstAccelerationStructure(*geometry.blas);
            build_info.setScratchData(
                vk::DeviceOrHostAddressKHR(scratch_address));
            vk::AccelerationStructureBuildRangeInfoKHR range_info(
                primitive_count,
                0,
                0,
                0);
            const vk::AccelerationStructureBuildRangeInfoKHR* range_infos[] = {
                &range_info};
            command_queue_->SubmitOneTime(
                [&](vk::CommandBuffer command_buffer) {
                    command_buffer.buildAccelerationStructuresKHR(
                        build_info,
                        range_infos);
                });

            geometry.blas_address =
                vk_unique_device_->getAccelerationStructureAddressKHR(
                    vk::AccelerationStructureDeviceAddressInfoKHR(
                        *geometry.blas));

            vk::AccelerationStructureInstanceKHR instance{};
            instance.transform = MakeIdentityTransform();
            instance.instanceCustomIndex = geometry.instance_custom_index;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = static_cast<VkGeometryInstanceFlagsKHR>(
                vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instance.accelerationStructureReference = geometry.blas_address;
            instances.push_back(instance);
            hardware_raytracing_geometries_.push_back(std::move(geometry));
        };

    if (use_source_instances)
    {
        for (const auto& source_geometry : source_geometries)
        {
            append_source_geometry(source_geometry);
        }
    }
    else
    {
        append_geometry("TriangleBufferTransmissive", 0u);
        append_geometry("TriangleBufferOpaque", 1u);
    }

    if (instances.empty())
    {
        use_hardware_raytracing_ = false;
        hardware_raytracing_uses_source_instances_ = false;
        logger_->warn(
            "No eligible meshes found for Vulkan hardware raytracing; falling back to software traversal.");
        return;
    }

    const vk::DeviceSize instance_bytes =
        static_cast<vk::DeviceSize>(
            instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    hardware_raytracing_instance_buffer_ = gpu_memory_manager_->CreateBuffer(
        instance_bytes,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        hardware_raytracing_instance_memory_,
        vk::MemoryAllocateFlagBits::eDeviceAddress);
    void* mapped_instances = vk_unique_device_->mapMemory(
        *hardware_raytracing_instance_memory_,
        0,
        instance_bytes);
    std::memcpy(mapped_instances, instances.data(), static_cast<std::size_t>(instance_bytes));
    vk_unique_device_->unmapMemory(*hardware_raytracing_instance_memory_);
    hardware_raytracing_instance_bytes_.resize(static_cast<std::size_t>(instance_bytes));
    if (instance_bytes > 0)
    {
        std::memcpy(
            hardware_raytracing_instance_bytes_.data(),
            instances.data(),
            static_cast<std::size_t>(instance_bytes));
    }

    UpdateHardwareRaytracingInstanceStorageBuffer();

    const auto instance_address = vk_unique_device_->getBufferAddress(
        vk::BufferDeviceAddressInfo(*hardware_raytracing_instance_buffer_));
    vk::AccelerationStructureGeometryInstancesDataKHR instances_data(
        VK_FALSE,
        vk::DeviceOrHostAddressConstKHR(instance_address));
    vk::AccelerationStructureGeometryDataKHR tlas_geometry_data;
    tlas_geometry_data.setInstances(instances_data);
    vk::AccelerationStructureGeometryKHR tlas_geometry(
        vk::GeometryTypeKHR::eInstances);
    tlas_geometry.setGeometry(tlas_geometry_data);

    vk::AccelerationStructureBuildGeometryInfoKHR tlas_build_info(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        GetHardwareRaytracingBuildFlags(),
        vk::BuildAccelerationStructureModeKHR::eBuild,
        {},
        {},
        tlas_geometry);
    const std::uint32_t instance_count =
        static_cast<std::uint32_t>(instances.size());
    const auto tlas_size = vk_unique_device_->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        tlas_build_info,
        instance_count);
    create_acceleration_structure(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        tlas_size.accelerationStructureSize,
        hardware_raytracing_tlas_,
        hardware_raytracing_tlas_buffer_,
        hardware_raytracing_tlas_memory_);

    vk::UniqueBuffer tlas_scratch_buffer;
    vk::UniqueDeviceMemory tlas_scratch_memory;
    const auto tlas_scratch_address = build_scratch_buffer(
        tlas_size.buildScratchSize,
        tlas_scratch_buffer,
        tlas_scratch_memory);
    tlas_build_info.setDstAccelerationStructure(*hardware_raytracing_tlas_);
    tlas_build_info.setScratchData(
        vk::DeviceOrHostAddressKHR(tlas_scratch_address));
    vk::AccelerationStructureBuildRangeInfoKHR tlas_range_info(
        instance_count,
        0,
        0,
        0);
    const vk::AccelerationStructureBuildRangeInfoKHR* tlas_ranges[] = {
        &tlas_range_info};
    command_queue_->SubmitOneTime(
        [&](vk::CommandBuffer command_buffer) {
            command_buffer.buildAccelerationStructuresKHR(
                tlas_build_info,
                tlas_ranges);
        });

    UpdateHardwareRaytracingInstanceStorageBuffer();

    logger_->info(
        "Built Vulkan hardware raytracing scene with {} instance(s).",
        instances.size());
    UpdateHardwareRaytracingDescriptor();
}

void Device::DestroyHardwareRaytracingScene()
{
    hardware_raytracing_tlas_.reset();
    hardware_raytracing_tlas_buffer_.reset();
    hardware_raytracing_tlas_memory_.reset();
    hardware_raytracing_instance_buffer_.reset();
    hardware_raytracing_instance_memory_.reset();
    hardware_raytracing_instance_bytes_.clear();
    hardware_raytracing_geometries_.clear();
    hardware_raytracing_uses_source_instances_ = false;
}

void Device::CreateComputeOutputImage()
{
    if (output_image_resources_)
    {
        output_image_resources_->CreateComputeOutputImage();
    }
}

void Device::DestroyComputeOutputImage()
{
    if (output_image_resources_)
    {
        output_image_resources_->DestroyComputeOutputImage();
    }
}

void Device::CreateSwapchainPreviewImage()
{
    if (output_image_resources_)
    {
        output_image_resources_->CreateSwapchainPreviewImage();
    }
}

void Device::DestroySwapchainPreviewImage()
{
    if (output_image_resources_)
    {
        output_image_resources_->DestroySwapchainPreviewImage();
    }
}


void Device::CreateTextureResources(
    const frame::json::LevelData& level_data)
{
    if (!level_ || !texture_resources_)
    {
        return;
    }

    texture_resources_->Build(*level_, level_data);
}

void Device::CreateDescriptorResources()
{
    descriptor_set_ = VK_NULL_HANDLE;
    descriptor_pool_.reset();
    descriptor_set_layout_.reset();
    storage_buffers_ready_ = false;

    if (!active_program_info_ || !buffer_resources_ || !texture_resources_)
    {
        return;
    }

    const auto& binding_defs = active_program_info_->bindings;
    if (binding_defs.empty())
    {
        return;
    }

    std::unordered_set<std::uint32_t> used_bindings;
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    layout_bindings.reserve(binding_defs.size());

    std::vector<std::uint32_t> texture_bindings;
    std::vector<EntityId> texture_ids;
    std::vector<std::uint32_t> storage_bindings;
    std::vector<EntityId> storage_ids;
    std::vector<std::uint32_t> uniform_bindings;
    std::vector<std::uint32_t> output_image_bindings;
    std::vector<std::uint32_t> output_sampler_bindings;
    bool needs_acceleration_structure = false;

    for (const auto& binding : binding_defs)
    {
        vk::ShaderStageFlags descriptor_stages = binding.stages;
        if (use_raytracing_pipeline_ &&
            (descriptor_stages & vk::ShaderStageFlagBits::eCompute) !=
                vk::ShaderStageFlags{})
        {
            descriptor_stages &= ~vk::ShaderStageFlagBits::eCompute;
            descriptor_stages |= vk::ShaderStageFlagBits::eRaygenKHR;
        }

        if (descriptor_stages == vk::ShaderStageFlags{})
        {
            logger_->error(
                "Program {} binding {} has no shader stages.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }
        if (!used_bindings.insert(binding.binding).second)
        {
            logger_->error(
                "Program {} has duplicate binding index {}.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }

        vk::DescriptorType descriptor_type{};
        switch (binding.binding_type)
        {
        case frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER:
            descriptor_type = vk::DescriptorType::eCombinedImageSampler;
            if (binding.name.empty())
            {
                logger_->error(
                    "Program {} texture binding {} has no name.",
                    active_program_info_->program_name,
                    binding.binding);
                return;
            }
            if (auto it =
                    active_program_info_->texture_ids_by_inner.find(binding.name);
                it != active_program_info_->texture_ids_by_inner.end())
            {
                texture_bindings.push_back(binding.binding);
                texture_ids.push_back(it->second);
            }
            else
            {
                logger_->error(
                    "Program {} missing texture '{}' for binding {}.",
                    active_program_info_->program_name,
                    binding.name,
                    binding.binding);
                return;
            }
            break;
        case frame::proto::ProgramBinding::OUTPUT_SAMPLER:
            descriptor_type = vk::DescriptorType::eCombinedImageSampler;
            output_sampler_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::STORAGE_IMAGE:
            descriptor_type = vk::DescriptorType::eStorageImage;
            output_image_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::STORAGE_BUFFER:
            descriptor_type = vk::DescriptorType::eStorageBuffer;
            if (binding.name.empty())
            {
                logger_->error(
                    "Program {} storage buffer binding {} has no name.",
                    active_program_info_->program_name,
                    binding.binding);
                return;
            }
            if (auto it =
                    active_program_info_->buffer_ids_by_inner.find(binding.name);
                it != active_program_info_->buffer_ids_by_inner.end())
            {
                storage_bindings.push_back(binding.binding);
                storage_ids.push_back(it->second);
            }
            else
            {
                logger_->error(
                    "Program {} missing buffer '{}' for binding {}.",
                    active_program_info_->program_name,
                    binding.name,
                    binding.binding);
                return;
            }
            break;
        case frame::proto::ProgramBinding::UNIFORM_BUFFER:
            descriptor_type = vk::DescriptorType::eUniformBuffer;
            uniform_bindings.push_back(binding.binding);
            break;
        case frame::proto::ProgramBinding::BINDING_INVALID:
        default:
            logger_->error(
                "Program {} has invalid binding type at index {}.",
                active_program_info_->program_name,
                binding.binding);
            return;
        }

        layout_bindings.emplace_back(
            binding.binding,
            descriptor_type,
            1,
            descriptor_stages);
    }

    if (use_hardware_raytracing_)
    {
        if (!hardware_raytracing_tlas_)
        {
            logger_->error(
                "Program {} requested Vulkan hardware raytracing without a TLAS.",
                active_program_info_->program_name);
            return;
        }
        if (!used_bindings.insert(kHardwareRaytracingAsBinding).second)
        {
            logger_->error(
                "Program {} collides with reserved hardware raytracing binding {}.",
                active_program_info_->program_name,
                kHardwareRaytracingAsBinding);
            return;
        }
        layout_bindings.emplace_back(
            kHardwareRaytracingAsBinding,
            vk::DescriptorType::eAccelerationStructureKHR,
            1,
            use_raytracing_pipeline_
                ? vk::ShaderStageFlagBits::eRaygenKHR
                : vk::ShaderStageFlagBits::eCompute);
        needs_acceleration_structure = true;
    }

    if (!output_image_bindings.empty() || !output_sampler_bindings.empty())
    {
        if (!use_compute_raytracing_)
        {
            logger_->error(
                "Program {} declares compute output bindings but compute is disabled.",
                active_program_info_->program_name);
            return;
        }
        if (output_image_bindings.empty())
        {
            logger_->error(
                "Program {} is missing a storage image binding for compute output.",
                active_program_info_->program_name);
            return;
        }
    }

    if (uniform_bindings.size() > 1)
    {
        logger_->error(
            "Program {} declares {} uniform bindings; only one is supported.",
            active_program_info_->program_name,
            uniform_bindings.size());
        return;
    }

    buffer_resources_->Clear();
    if (!storage_ids.empty())
    {
        if (!level_)
        {
            logger_->error(
                "Program {} requested buffers without a level.",
                active_program_info_->program_name);
            return;
        }
        buffer_resources_->BuildStorageBuffers(*level_, storage_ids);
    }

    const auto& storage_buffers = buffer_resources_->GetStorageBuffers();
    if (storage_buffers.size() != storage_ids.size())
    {
        logger_->error(
            "Program {} storage buffer count mismatch ({} vs {}).",
            active_program_info_->program_name,
            storage_buffers.size(),
            storage_ids.size());
        return;
    }


    const BufferResource* uniform = nullptr;
    if (!uniform_bindings.empty())
    {
        buffer_resources_->BuildUniformBuffer(
            static_cast<vk::DeviceSize>(sizeof(UniformBlock)));
        uniform = buffer_resources_->GetUniformBuffer();
        if (!uniform)
        {
            logger_->error("Failed to allocate Vulkan uniform buffer.");
            return;
        }
    }

    std::vector<vk::DescriptorImageInfo> texture_infos;
    if (!texture_ids.empty())
    {
        if (texture_resources_->Empty())
        {
            logger_->error(
                "Program {} requires textures but none are loaded.",
                active_program_info_->program_name);
            return;
        }
        std::vector<EntityId> resolved_ids;
        if (!texture_resources_->CollectDescriptorInfos(
                texture_ids,
                texture_infos,
                resolved_ids))
        {
            return;
        }
        if (texture_infos.size() != texture_ids.size())
        {
            logger_->error(
                "Program {} texture descriptor count mismatch.",
                active_program_info_->program_name);
            return;
        }
    }

    const bool needs_compute_output =
        !output_image_bindings.empty() || !output_sampler_bindings.empty();
    if (needs_compute_output)
    {
        DestroyComputeOutputImage();
        if (!swapchain_resources_ || !swapchain_resources_->IsValid())
        {
            return;
        }
        const auto extent = swapchain_resources_->GetExtent();
        if (extent.width == 0 || extent.height == 0)
        {
            return;
        }
        CreateComputeOutputImage();
        if (!output_image_resources_ ||
            !output_image_resources_->HasComputeOutputView())
        {
            logger_->error(
                "Failed to create compute output image for program {}.",
                active_program_info_->program_name);
            return;
        }
    }

    vk::DescriptorSetLayoutCreateInfo layout_info(
        vk::DescriptorSetLayoutCreateFlags{},
        static_cast<std::uint32_t>(layout_bindings.size()),
        layout_bindings.data());
    descriptor_set_layout_ =
        vk_unique_device_->createDescriptorSetLayoutUnique(layout_info);

    std::vector<vk::DescriptorPoolSize> pool_sizes;
    const std::uint32_t combined_count =
        static_cast<std::uint32_t>(texture_bindings.size() +
                                   output_sampler_bindings.size());
    if (combined_count > 0)
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler,
            combined_count);
    }
    if (!output_image_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageImage,
            static_cast<std::uint32_t>(output_image_bindings.size()));
    }
    if (!storage_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eStorageBuffer,
            static_cast<std::uint32_t>(storage_bindings.size()));
    }
    if (!uniform_bindings.empty())
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eUniformBuffer,
            static_cast<std::uint32_t>(uniform_bindings.size()));
    }
    if (needs_acceleration_structure)
    {
        pool_sizes.emplace_back(
            vk::DescriptorType::eAccelerationStructureKHR,
            1);
    }
    if (pool_sizes.empty())
    {
        return;
    }

    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlags{},
        1,
        static_cast<std::uint32_t>(pool_sizes.size()),
        pool_sizes.data());
    descriptor_pool_ = vk_unique_device_->createDescriptorPoolUnique(pool_info);

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::DescriptorSetAllocateInfo alloc_info(
        *descriptor_pool_,
        1,
        layouts);
    descriptor_set_ =
        vk_unique_device_->allocateDescriptorSets(alloc_info).front();

    std::vector<vk::DescriptorImageInfo> output_image_infos;
    std::vector<vk::DescriptorImageInfo> output_sampler_infos;
    std::vector<vk::DescriptorBufferInfo> storage_infos;
    std::vector<vk::DescriptorBufferInfo> uniform_infos;
    std::vector<vk::WriteDescriptorSet> descriptor_writes;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR>
        acceleration_writes;
    std::vector<vk::AccelerationStructureKHR> acceleration_handles;
    descriptor_writes.reserve(layout_bindings.size());
    output_image_infos.reserve(output_image_bindings.size());
    output_sampler_infos.reserve(output_sampler_bindings.size());
    storage_infos.reserve(storage_buffers.size());
    uniform_infos.reserve(uniform_bindings.size());
    acceleration_writes.reserve(needs_acceleration_structure ? 1u : 0u);
    acceleration_handles.reserve(needs_acceleration_structure ? 1u : 0u);

    for (auto binding : output_image_bindings)
    {
        output_image_infos.emplace_back(
            nullptr,
            output_image_resources_->GetComputeOutputView(),
            vk::ImageLayout::eGeneral);
        descriptor_writes.emplace_back(
            descriptor_set_,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &output_image_infos.back());
    }

    for (auto binding : output_sampler_bindings)
    {
        output_sampler_infos.emplace_back(
            output_image_resources_->GetComputeOutputSampler(),
            output_image_resources_->GetComputeOutputView(),
            vk::ImageLayout::eShaderReadOnlyOptimal);
        descriptor_writes.emplace_back(
            descriptor_set_,
            binding,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &output_sampler_infos.back());
    }

    for (std::size_t i = 0; i < texture_infos.size(); ++i)
    {
        descriptor_writes.emplace_back(
            descriptor_set_,
            texture_bindings[i],
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texture_infos[i]);
    }

    for (std::size_t i = 0; i < storage_buffers.size(); ++i)
    {
        storage_infos.emplace_back(
            *storage_buffers[i].buffer,
            0,
            storage_buffers[i].size);
        descriptor_writes.emplace_back(
            descriptor_set_,
            storage_bindings[i],
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &storage_infos.back());
    }

    if (uniform && !uniform_bindings.empty())
    {
        uniform_infos.emplace_back(
            *uniform->buffer,
            0,
            uniform->size);
        descriptor_writes.emplace_back(
            descriptor_set_,
            uniform_bindings.front(),
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &uniform_infos.back());
    }

    if (needs_acceleration_structure)
    {
        acceleration_handles.push_back(hardware_raytracing_tlas_.get());
        acceleration_writes.emplace_back(
            1,
            acceleration_handles.data());
        descriptor_writes.emplace_back(
            descriptor_set_,
            kHardwareRaytracingAsBinding,
            0,
            1,
            vk::DescriptorType::eAccelerationStructureKHR);
        descriptor_writes.back().setPNext(&acceleration_writes.back());
    }

    vk_unique_device_->updateDescriptorSets(
        static_cast<std::uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(),
        0,
        nullptr);
}


} // namespace frame::vulkan




