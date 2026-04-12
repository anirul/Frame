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

bool RaytraceSceneRequiresWorldSpaceBuffers(frame::LevelInterface& level)
{
    for (const auto& [node_id, material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)material_id;
        auto* node =
            dynamic_cast<frame::NodeMesh*>(&level.GetSceneNodeFromId(node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        auto* skinned_mesh = dynamic_cast<frame::vulkan::SkinnedMesh*>(
            &level.GetMeshFromId(mesh_id));
        if (!skinned_mesh)
        {
            continue;
        }
        if (skinned_mesh->IsSkinningAnimationEnabled() ||
            skinned_mesh->HasRaytraceTriangleCallback())
        {
            return true;
        }
    }
    return false;
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
      texture_resources_(std::make_unique<TextureResources>(*this))
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
    use_procedural_quad_pipeline_ = false;
    use_compute_raytracing_ = false;
    use_raytracing_pipeline_ = false;
    compute_output_in_shader_read_ = false;
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
    use_procedural_quad_pipeline_ = false;
    use_raytracing_pipeline_ = false;
    push_constant_stages_ = {};
    push_constant_size_ = 0;
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
        if (triangle_buffer_id && skinned_mesh->HasRaytraceTriangleCallback())
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
                if (buffer_resources_->UpdateStorageBuffer(
                        level_->GetNameFromId(triangle_buffer_id),
                        triangle_buffer.GetRawData()))
                {
                    ++updated_buffer_count;
                }
            }
        }

        const auto bvh_buffer_id = skinned_mesh->GetBvhBufferId();
        if (bvh_buffer_id && skinned_mesh->HasRaytraceBvhCallback())
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
    if (RaytraceSceneRequiresWorldSpaceBuffers(*level_))
    {
        updated_aggregate_scene = UpdateAggregateRaytracingSceneBuffers(
            !use_hardware_raytracing_);
        if (use_hardware_raytracing_ && updated_aggregate_scene)
        {
            UpdateHardwareRaytracingScene();
        }
    }
    if (updated_buffer_count > 0 || updated_aggregate_scene)
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
    const std::size_t scene_state_hash =
        BuildRaytracingSourceStateHash(
            *level_,
            static_cast<double>(elapsed_time_seconds_));
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
            static_cast<double>(elapsed_time_seconds_));
    const auto opaque_triangles =
        BuildAggregateTriangleBytes(
            *level_,
            false,
            static_cast<double>(elapsed_time_seconds_));

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

void Device::UpdateHardwareRaytracingScene()
{
    if (!use_hardware_raytracing_ || !vk_unique_device_ || !level_ ||
        !gpu_memory_manager_ || !command_queue_ ||
        hardware_raytracing_geometries_.empty() || !hardware_raytracing_tlas_)
    {
        return;
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
                    "vkWaitForFences failed before updating hardware raytracing scene: {}",
                    vk::to_string(static_cast<vk::Result>(wait_result)));
                if (wait_result == VK_ERROR_DEVICE_LOST)
                {
                    device_lost_ = true;
                }
                return;
            }
        }
    }

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

    for (auto& geometry : hardware_raytracing_geometries_)
    {
        if (!geometry.vertex_buffer || !geometry.index_buffer || !geometry.blas)
        {
            continue;
        }

        auto* triangle_buffer = dynamic_cast<frame::vulkan::Buffer*>(
            &level_->GetBufferFromId(geometry.source_buffer_id));
        if (!triangle_buffer)
        {
            continue;
        }

        const auto& triangle_bytes = triangle_buffer->GetRawData();
        if (triangle_bytes.empty())
        {
            continue;
        }
        if (static_cast<vk::DeviceSize>(triangle_bytes.size()) !=
                geometry.vertex_buffer_size ||
            triangle_bytes.size() % kRaytraceTriangleVertexStrideBytes != 0)
        {
            logger_->info(
                "Recreating Vulkan hardware raytracing scene because animated geometry '{}' changed layout.",
                geometry.source_inner_name);
            CreateHardwareRaytracingScene();
            UpdateHardwareRaytracingDescriptor();
            return;
        }

        UploadDeviceLocalBuffer(
            *vk_unique_device_,
            *gpu_memory_manager_,
            *command_queue_,
            triangle_bytes,
            *geometry.vertex_buffer);

        const auto vertex_address = vk_unique_device_->getBufferAddress(
            vk::BufferDeviceAddressInfo(*geometry.vertex_buffer));
        const auto index_address = vk_unique_device_->getBufferAddress(
            vk::BufferDeviceAddressInfo(*geometry.index_buffer));

        vk::AccelerationStructureGeometryTrianglesDataKHR triangles(
            vk::Format::eR32G32B32Sfloat,
            vk::DeviceOrHostAddressConstKHR(vertex_address),
            kRaytraceTriangleVertexStrideBytes,
            geometry.vertex_count,
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
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {},
            {},
            as_geometry);

        const auto size_info =
            vk_unique_device_->getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                build_info,
                geometry.triangle_count);
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
            geometry.triangle_count,
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
    }

    const std::uint32_t instance_count =
        static_cast<std::uint32_t>(hardware_raytracing_geometries_.size());
    if (instance_count == 0 || !hardware_raytracing_instance_buffer_)
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
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        vk::BuildAccelerationStructureModeKHR::eBuild,
        {},
        {},
        tlas_geometry);
    const auto tlas_size = vk_unique_device_->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        tlas_build_info,
        instance_count);
    vk::UniqueBuffer tlas_scratch_buffer;
    vk::UniqueDeviceMemory tlas_scratch_memory;
    const auto tlas_scratch_address = build_scratch_address(
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
    if (!compute_output_sampler_ || !compute_output_view_)
    {
        return std::nullopt;
    }
    return vk::DescriptorImageInfo(
        *compute_output_sampler_,
        *compute_output_view_,
        vk::ImageLayout::eShaderReadOnlyOptimal);
}

std::optional<vk::DescriptorImageInfo> Device::GetSwapchainPreviewDescriptorInfo() const
{
    if (!swapchain_preview_in_shader_read_ || !swapchain_preview_sampler_ ||
        !swapchain_preview_view_)
    {
        return std::nullopt;
    }
    return vk::DescriptorImageInfo(
        *swapchain_preview_sampler_,
        *swapchain_preview_view_,
        vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Device::Display(double dt)
{
    if (device_lost_)
    {
        return;
    }

    elapsed_time_seconds_ += static_cast<float>(dt);

    if (level_)
    {
        level_->UpdateLights(static_cast<double>(elapsed_time_seconds_));
        UpdateRaytraceBuffers();
    }

    if (!vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }
    if (!command_resources_ || command_resources_->GetBuffers().empty())
    {
        return;
    }
    if (!sync_resources_ || !sync_resources_->IsCreated())
    {
        return;
    }
    const bool has_scene_pipeline =
        static_cast<bool>(graphics_pipeline_) &&
        static_cast<bool>(pipeline_layout_);
    const bool has_gui_render = static_cast<bool>(gui_render_callback_);
    if (!has_scene_pipeline && !has_gui_render)
    {
        return;
    }

    if (framebuffer_resized_)
    {
        framebuffer_resized_ = false;
        RecreateSwapchain();
        return;
    }

    const vk::Fence fence = sync_resources_->GetInFlightFence(current_frame_);
    const VkFence fence_handle = static_cast<VkFence>(fence);
    const VkResult wait_result = vkWaitForFences(
        static_cast<VkDevice>(*vk_unique_device_),
        1,
        &fence_handle,
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());
    if (wait_result != VK_SUCCESS)
    {
        logger_->error(
            "vkWaitForFences failed: {}",
            vk::to_string(static_cast<vk::Result>(wait_result)));
        if (wait_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    const auto& swapchain = swapchain_resources_->GetSwapchain();
    auto acquire = vk_unique_device_->acquireNextImageKHR(
        *swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        sync_resources_->GetImageAvailable(current_frame_),
        nullptr);

    if (acquire.result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapchain();
        return;
    }
    if (acquire.result != vk::Result::eSuccess &&
        acquire.result != vk::Result::eSuboptimalKHR)
    {
        logger_->error(
            "Failed to acquire swapchain image: {}",
            vk::to_string(acquire.result));
        if (acquire.result == vk::Result::eErrorDeviceLost)
        {
            device_lost_ = true;
        }
        return;
    }

    const std::uint32_t image_index = acquire.value;
    const VkResult reset_result = vkResetFences(
        static_cast<VkDevice>(*vk_unique_device_),
        1,
        &fence_handle);
    if (reset_result != VK_SUCCESS)
    {
        logger_->error(
            "vkResetFences failed: {}",
            vk::to_string(static_cast<vk::Result>(reset_result)));
        if (reset_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    vk::CommandBuffer command_buffer =
        command_resources_->GetBuffer(current_frame_);
    command_buffer.reset();
    RecordCommandBuffer(command_buffer, image_index);

    const vk::Semaphore wait_semaphores[] = {
        sync_resources_->GetImageAvailable(current_frame_)};
    const vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::Semaphore signal_semaphores[] = {
        sync_resources_->GetRenderFinished(image_index)};

    vk::SubmitInfo submit_info(
        1,
        wait_semaphores,
        wait_stages,
        1,
        &command_buffer,
        1,
        signal_semaphores);

    const VkSubmitInfo submit_info_c = submit_info;
    const VkResult submit_result = vkQueueSubmit(
        static_cast<VkQueue>(graphics_queue_),
        1,
        &submit_info_c,
        fence);
    if (submit_result != VK_SUCCESS)
    {
        logger_->error(
            "vkQueueSubmit failed: {}",
            vk::to_string(static_cast<vk::Result>(submit_result)));
        if (submit_result == VK_ERROR_DEVICE_LOST)
        {
            device_lost_ = true;
        }
        return;
    }

    vk::PresentInfoKHR present_info(
        1,
        signal_semaphores,
        1,
        &swapchain.get(),
        &image_index);

    const vk::Result present_result = present_queue_.presentKHR(present_info);
    if (present_result == vk::Result::eErrorOutOfDateKHR ||
        present_result == vk::Result::eSuboptimalKHR)
    {
        RecreateSwapchain();
    }
    else if (present_result != vk::Result::eSuccess)
    {
        logger_->error(
            "Failed to present swapchain image: {}",
            vk::to_string(present_result));
        if (present_result == vk::Result::eErrorDeviceLost)
        {
            device_lost_ = true;
        }
        return;
    }

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
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
        static_cast<bool>(raytracing_pipeline_);
    const bool compute_pipeline_ready =
        static_cast<bool>(compute_pipeline_);
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

void Device::RecordCommandBuffer(
    vk::CommandBuffer command_buffer,
    std::uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info;
    command_buffer.begin(begin_info);

    const auto extent = swapchain_resources_->GetExtent();
    const auto& images = swapchain_resources_->GetImages();
    const auto& render_pass = swapchain_resources_->GetRenderPass();
    const auto& framebuffers = swapchain_resources_->GetFramebuffers();
    const auto& gui_render_pass = swapchain_resources_->GetGuiRenderPass();
    const auto& gui_framebuffers = swapchain_resources_->GetGuiFramebuffers();

    const bool use_world_space_raytrace_scene =
        level_ &&
        (use_compute_raytracing_ || use_raytracing_pipeline_) &&
        RaytraceSceneRequiresWorldSpaceBuffers(*level_);
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

    const SceneState scene_state =
        (level_)
            ? BuildSceneState(
                  *level_,
                  frame::Logger::GetInstance(),
                  {extent.width, extent.height},
                  elapsed_time_seconds_,
                  active_program_info_
                      ? active_program_info_->material_id
                      : NullId,
                  !use_compute_raytracing_,
                  preferred_scene_root,
                  use_world_space_raytrace_scene)
            : SceneState{};

    auto update_uniform_buffer = [&](const SceneState& state) {
        if (!buffer_resources_)
        {
            return;
        }
        if (!buffer_resources_->GetUniformBuffer())
        {
            return;
        }
        auto block = MakeUniformBlock(
            state, elapsed_time_seconds_);
        buffer_resources_->UpdateUniform(
            &block, sizeof(UniformBlock));
    };
    update_uniform_buffer(scene_state);

    auto transition_output = [&](vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout,
                                 vk::PipelineStageFlags src_stage,
                                 vk::PipelineStageFlags dst_stage,
                                 vk::AccessFlags src_access,
                                 vk::AccessFlags dst_access) {
        if (!compute_output_image_)
        {
            return;
        }
        vk::ImageMemoryBarrier barrier(
            src_access,
            dst_access,
            old_layout,
            new_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            *compute_output_image_,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            nullptr,
            nullptr,
            barrier);
    };

    if (use_compute_raytracing_ &&
        descriptor_set_ &&
        compute_output_image_ &&
        extent.width > 0 &&
        extent.height > 0)
    {
        if (!storage_buffers_ready_ && buffer_resources_)
        {
            const auto& storage_buffers =
                buffer_resources_->GetStorageBuffers();
            std::vector<vk::BufferMemoryBarrier> buffer_barriers;
            buffer_barriers.reserve(storage_buffers.size());
            for (const auto& storage : storage_buffers)
            {
                if (!storage.buffer || storage.size == 0)
                {
                    continue;
                }
                buffer_barriers.emplace_back(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eShaderRead,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    *storage.buffer,
                    0,
                    storage.size);
            }
            if (!buffer_barriers.empty())
            {
                command_buffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    use_raytracing_pipeline_
                        ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                        : vk::PipelineStageFlagBits::eComputeShader,
                    {},
                    nullptr,
                    buffer_barriers,
                    nullptr);
            }
            storage_buffers_ready_ = true;
        }

        if (compute_output_in_shader_read_)
        {
            transition_output(
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eFragmentShader,
                use_raytracing_pipeline_
                    ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                    : vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eShaderWrite);
            compute_output_in_shader_read_ = false;
        }

        if (use_raytracing_pipeline_ &&
            raytracing_pipeline_ &&
            raytracing_pipeline_layout_)
        {
            command_buffer.bindPipeline(
                vk::PipelineBindPoint::eRayTracingKHR,
                *raytracing_pipeline_);
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR,
                *raytracing_pipeline_layout_,
                0,
                descriptor_set_,
                {});
            command_buffer.traceRaysKHR(
                raygen_sbt_region_,
                miss_sbt_region_,
                hit_sbt_region_,
                callable_sbt_region_,
                extent.width,
                extent.height,
                1);
        }
        else if (compute_pipeline_ && compute_pipeline_layout_)
        {
            command_buffer.bindPipeline(
                vk::PipelineBindPoint::eCompute,
                *compute_pipeline_);
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute,
                *compute_pipeline_layout_,
                0,
                descriptor_set_,
                {});
            const std::uint32_t group_x =
                (extent.width + 7) / 8;
            const std::uint32_t group_y =
                (extent.height + 7) / 8;
            command_buffer.dispatch(group_x, group_y, 1);
        }

        transition_output(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            use_raytracing_pipeline_
                ? vk::PipelineStageFlagBits::eRayTracingShaderKHR
                : vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
        compute_output_in_shader_read_ = true;
    }

    std::array<vk::ClearValue, 1> clear_values{};
    clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{
        0.1f,
        0.1f,
        0.1f,
        1.0f});

    auto draw_scene = [&]() -> bool {
        if (!graphics_pipeline_)
        {
            return false;
        }
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *graphics_pipeline_);

        vk::Viewport viewport(
            0.0f,
            0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f,
            1.0f);
        command_buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor({0, 0}, extent);
        command_buffer.setScissor(0, 1, &scissor);

        if (descriptor_set_layout_ && descriptor_set_)
        {
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipeline_layout_,
                0,
                descriptor_set_,
                {});
        }

        glm::mat4 projection = scene_state.projection;
        glm::mat4 view = scene_state.view;
        glm::mat4 model = scene_state.model;

        const bool needs_scene_matrices =
            push_constant_size_ > 0 &&
            !(use_procedural_quad_pipeline_ &&
              active_program_info_ &&
              active_program_info_->uses_time_uniform);

        if (needs_scene_matrices && level_)
        {
            try
            {
                Camera camera_for_frame(level_->GetDefaultCamera());
                auto camera_holder_id = level_->GetDefaultCameraId();
                if (camera_holder_id != NullId)
                {
                    auto& node =
                        level_->GetSceneNodeFromId(camera_holder_id);
                    auto matrix_node = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds_));
                    auto inverse_model = glm::inverse(matrix_node);
                    camera_for_frame.SetFront(
                        level_->GetDefaultCamera().GetFront() *
                        glm::mat3(inverse_model));
                    camera_for_frame.SetPosition(
                        glm::vec3(
                            glm::vec4(
                                level_->GetDefaultCamera().GetPosition(), 1.0f) *
                            inverse_model));
                }

                if (extent.height != 0)
                {
                    camera_for_frame.SetAspectRatio(
                        static_cast<float>(extent.width) /
                        static_cast<float>(extent.height));
                }
                projection = camera_for_frame.ComputeProjection();
                projection[1][1] *= -1.0f;
                view = camera_for_frame.ComputeView();
                glm::mat4 rotation = glm::mat4(1.0f);
                view = rotation * view;

                const auto mesh_pairs =
                    level_->GetMeshMaterialIds();
                if (!mesh_pairs.empty())
                {
                    auto node_id = mesh_pairs.front().first;
                    auto& node = level_->GetSceneNodeFromId(node_id);
                    model = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds_));
                }
            }
            catch (const std::exception& ex)
            {
                logger_->warn(
                    "Failed to compute scene matrices: {}", ex.what());
            }
        }

        if (push_constant_size_ > 0)
        {
            if (use_procedural_quad_pipeline_ && active_program_info_ &&
                active_program_info_->uses_time_uniform)
            {
                float time = elapsed_time_seconds_;
                command_buffer.pushConstants(
                    *pipeline_layout_,
                    push_constant_stages_,
                    0,
                    push_constant_size_,
                    &time);
            }
            else
            {
                struct alignas(16) PushConstants
                {
                    glm::mat4 projection;
                    glm::mat4 view;
                    glm::mat4 model;
                    float time_s;
                } push_constants{projection, view, model, elapsed_time_seconds_};
                command_buffer.pushConstants(
                    *pipeline_layout_,
                    push_constant_stages_,
                    0,
                    push_constant_size_,
                    &push_constants);
            }
        }

        if (use_procedural_quad_pipeline_)
        {
            command_buffer.draw(6, 1, 0, 0);
            return true;
        }
        else if (mesh_resources_ && !mesh_resources_->Empty())
        {
            const auto& mesh = mesh_resources_->GetMeshes().front();
            const vk::DeviceSize offsets[] = {0};
            command_buffer.bindVertexBuffers(0, *mesh.vertex_buffer, offsets);
            if (mesh.index_buffer)
            {
                command_buffer.bindIndexBuffer(
                    *mesh.index_buffer, 0, vk::IndexType::eUint32);
                command_buffer.drawIndexed(mesh.index_count, 1, 0, 0, 0);
            }
            else
            {
                command_buffer.draw(mesh.index_count, 1, 0, 0);
            }
            return true;
        }
        return false;
    };

    bool scene_pass_executed = false;
    bool scene_content_rendered = false;
    if (render_pass && image_index < framebuffers.size())
    {
        vk::RenderPassBeginInfo render_pass_info(
            *render_pass,
            *framebuffers[image_index],
            vk::Rect2D({0, 0}, extent),
            static_cast<std::uint32_t>(clear_values.size()),
            clear_values.data());

        command_buffer.beginRenderPass(
            render_pass_info,
            vk::SubpassContents::eInline);
        scene_content_rendered = draw_scene();
        command_buffer.endRenderPass();
        scene_pass_executed = true;
    }

    if (!scene_pass_executed && image_index < images.size())
    {
        const vk::Image swapchain_image = images[image_index];
        vk::ImageMemoryBarrier to_transfer_src(
            {},
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain_image,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            to_transfer_src);
    }

    if (swapchain_preview_image_ && scene_content_rendered &&
        image_index < images.size() &&
        extent.width > 0 && extent.height > 0)
    {
        const vk::Image swapchain_image = images[image_index];

        std::array<vk::ImageMemoryBarrier, 2> to_copy_barriers = {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                swapchain_image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
            vk::ImageMemoryBarrier(
                swapchain_preview_in_shader_read_
                    ? vk::AccessFlagBits::eShaderRead
                    : vk::AccessFlags{},
                vk::AccessFlagBits::eTransferWrite,
                swapchain_preview_in_shader_read_
                    ? vk::ImageLayout::eShaderReadOnlyOptimal
                    : vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *swapchain_preview_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})};

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            to_copy_barriers);

        vk::ImageCopy copy_region(
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {0, 0, 0},
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {0, 0, 0},
            {extent.width, extent.height, 1});
        command_buffer.copyImage(
            swapchain_image,
            vk::ImageLayout::eTransferSrcOptimal,
            *swapchain_preview_image_,
            vk::ImageLayout::eTransferDstOptimal,
            copy_region);

        std::array<vk::ImageMemoryBarrier, 2> from_copy_barriers = {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferRead,
                gui_render_callback_
                    ? vk::AccessFlagBits::eTransferRead
                    : vk::AccessFlags{},
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                swapchain_image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *swapchain_preview_image_,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})};

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAllCommands,
            {},
            nullptr,
            nullptr,
            from_copy_barriers);

        swapchain_preview_in_shader_read_ = true;
    }

    if (gui_render_callback_ && gui_render_pass &&
        image_index < gui_framebuffers.size())
    {
        vk::RenderPassBeginInfo gui_pass_info(
            *gui_render_pass,
            *gui_framebuffers[image_index],
            vk::Rect2D({0, 0}, extent),
            0,
            nullptr);
        command_buffer.beginRenderPass(
            gui_pass_info,
            vk::SubpassContents::eInline);
        try
        {
            gui_render_callback_(command_buffer);
        }
        catch (const std::exception& ex)
        {
            logger_->error("Failed to render Vulkan GUI: {}", ex.what());
        }
        command_buffer.endRenderPass();
    }
    else if (image_index < images.size())
    {
        const vk::Image swapchain_image = images[image_index];
        vk::ImageMemoryBarrier to_present(
            vk::AccessFlagBits::eTransferRead,
            {},
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain_image,
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {},
            nullptr,
            nullptr,
            to_present);
    }

    command_buffer.end();
}

void Device::CreateGraphicsPipeline()
{
    if (!vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }

    const auto& render_pass = swapchain_resources_->GetRenderPass();

    DestroyGraphicsPipeline();

    if (!texture_resources_ || texture_resources_->Empty())
    {
        return;
    }

    std::vector<std::uint32_t> vert_code;
    std::vector<std::uint32_t> frag_code;
    if (!active_program_info_)
    {
        logger_->error(
            "No Vulkan program selected; cannot build graphics pipeline.");
        return;
    }
    if (active_program_info_->vertex_shader.empty() ||
        active_program_info_->fragment_shader.empty())
    {
        logger_->error(
            "Vulkan program {} is missing shader filenames.",
            active_program_info_->program_name);
        return;
    }

    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    try
    {
        vert_code = shader_compiler_->CompileFile(
            active_program_info_->vertex_shader, shaderc_vertex_shader);
        frag_code = shader_compiler_->CompileFile(
            active_program_info_->fragment_shader, shaderc_fragment_shader);
    }
    catch (const std::exception& ex)
    {
        logger_->warn(
            "Failed to compile Vulkan shader pair ({} / {}): {}",
            active_program_info_->vertex_shader.string(),
            active_program_info_->fragment_shader.string(),
            ex.what());
        return;
    }

    use_procedural_quad_pipeline_ =
        active_program_info_->scene_type == frame::proto::SceneType::QUAD;

    struct alignas(16) PushConstants
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        float time_s;
    };

    if (use_procedural_quad_pipeline_)
    {
        if (active_program_info_ && active_program_info_->uses_time_uniform)
        {
            push_constant_size_ = sizeof(float);
            push_constant_stages_ = vk::ShaderStageFlagBits::eFragment;
        }
        else
        {
            push_constant_size_ = 0;
            push_constant_stages_ = {};
        }
    }
    else
    {
        push_constant_size_ = static_cast<std::uint32_t>(sizeof(PushConstants));
        push_constant_stages_ = vk::ShaderStageFlagBits::eVertex;
    }

    auto vert_module = CreateShaderModule(vert_code);
    auto frag_module = CreateShaderModule(frag_code);

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, *vert_module, "main"},
        {vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, *frag_module, "main"},
    };

    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

    if (!use_procedural_quad_pipeline_)
    {
        binding_descriptions.emplace_back(
            0,
            static_cast<std::uint32_t>(sizeof(MeshVertex)),
            vk::VertexInputRate::eVertex);
        attribute_descriptions.emplace_back(
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, position)));
        attribute_descriptions.emplace_back(
            1,
            0,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, uv)));
    }

    vk::PipelineVertexInputStateCreateInfo vertex_input_info(
        vk::PipelineVertexInputStateCreateFlags{},
        static_cast<std::uint32_t>(binding_descriptions.size()),
        binding_descriptions.data(),
        static_cast<std::uint32_t>(attribute_descriptions.size()),
        attribute_descriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo input_assembly(
        vk::PipelineInputAssemblyStateCreateFlags{},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE);

    vk::PipelineViewportStateCreateInfo viewport_state(
        vk::PipelineViewportStateCreateFlags{},
        1,
        nullptr,
        1,
        nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags{},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags{},
        vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo color_blending(
        vk::PipelineColorBlendStateCreateFlags{},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1,
        &color_blend_attachment);

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state(
        vk::PipelineDynamicStateCreateFlags{},
        static_cast<std::uint32_t>(dynamic_states.size()),
        dynamic_states.data());

    std::vector<vk::DescriptorSetLayout> set_layouts;
    if (descriptor_set_layout_)
    {
        set_layouts.push_back(*descriptor_set_layout_);
    }

    std::vector<vk::PushConstantRange> push_constant_ranges;
    if (push_constant_size_ > 0)
    {
        push_constant_ranges.emplace_back(
            push_constant_stages_, 0, push_constant_size_);
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info(
        vk::PipelineLayoutCreateFlags{},
        static_cast<std::uint32_t>(set_layouts.size()),
        set_layouts.data(),
        static_cast<std::uint32_t>(push_constant_ranges.size()),
        push_constant_ranges.empty() ? nullptr : push_constant_ranges.data());
    pipeline_layout_ = vk_unique_device_->createPipelineLayoutUnique(pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        static_cast<std::uint32_t>(std::size(shader_stages)),
        shader_stages,
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterizer,
        &multisampling,
        nullptr,
        &color_blending,
        &dynamic_state,
        *pipeline_layout_,
        *render_pass);

    auto pipeline_result =
        vk_unique_device_->createGraphicsPipelineUnique(nullptr, pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create Vulkan graphics pipeline.");
    }
    graphics_pipeline_ = std::move(pipeline_result.value);
}

void Device::DestroyGraphicsPipeline()
{
    graphics_pipeline_.reset();
    pipeline_layout_.reset();
}

void Device::CreateComputePipeline()
{
    if (!use_compute_raytracing_ || !vk_unique_device_)
    {
        return;
    }

    DestroyComputePipeline();

    if (!descriptor_set_layout_)
    {
        logger_->warn(
            "CreateComputePipeline skipped: descriptor set layout missing.");
        return;
    }

    if (!active_program_info_ ||
        active_program_info_->compute_shader.empty())
    {
        logger_->warn(
            "CreateComputePipeline skipped: no active compute shader.");
        return;
    }

    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    std::vector<std::uint32_t> compute_code;
    try
    {
        compute_code = shader_compiler_->CompileFile(
            active_program_info_->compute_shader, shaderc_compute_shader);
    }
    catch (const std::exception& ex)
    {
        logger_->warn(
            "Failed to compile compute shader {}: {}",
            active_program_info_->compute_shader.string(),
            ex.what());
        return;
    }

    auto compute_module = CreateShaderModule(compute_code);

    vk::PipelineShaderStageCreateInfo stage_info(
        vk::PipelineShaderStageCreateFlags{},
        vk::ShaderStageFlagBits::eCompute,
        *compute_module,
        "main");

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{},
        1,
        layouts);
    compute_pipeline_layout_ =
        vk_unique_device_->createPipelineLayoutUnique(layout_info);

    vk::ComputePipelineCreateInfo pipeline_info(
        vk::PipelineCreateFlags{},
        stage_info,
        *compute_pipeline_layout_);

    auto pipeline_result =
        vk_unique_device_->createComputePipelineUnique(nullptr, pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create Vulkan compute pipeline.");
    }
    compute_pipeline_ = std::move(pipeline_result.value);
    logger_->info("Created raytracing compute pipeline.");
}

void Device::DestroyComputePipeline()
{
    compute_pipeline_.reset();
    compute_pipeline_layout_.reset();
    compute_output_in_shader_read_ = false;
}

void Device::CreateRaytracingPipeline()
{
    if (!use_raytracing_pipeline_ || !vk_unique_device_)
    {
        return;
    }

    DestroyRaytracingPipeline();

    if (!descriptor_set_layout_)
    {
        logger_->warn(
            "CreateRaytracingPipeline skipped: descriptor set layout missing.");
        return;
    }
    if (!active_program_info_ ||
        active_program_info_->raygen_shader.empty() ||
        active_program_info_->miss_shader.empty() ||
        active_program_info_->closesthit_shader.empty())
    {
        logger_->warn(
            "CreateRaytracingPipeline skipped: missing ray tracing shader stage.");
        return;
    }
    if (!shader_compiler_)
    {
        shader_compiler_ = std::make_unique<ShaderCompiler>();
    }

    std::vector<std::uint32_t> raygen_code;
    std::vector<std::uint32_t> miss_code;
    std::vector<std::uint32_t> closesthit_code;
    try
    {
        raygen_code = shader_compiler_->CompileFile(
            active_program_info_->raygen_shader,
            shaderc_raygen_shader);
        miss_code = shader_compiler_->CompileFile(
            active_program_info_->miss_shader,
            shaderc_miss_shader);
        closesthit_code = shader_compiler_->CompileFile(
            active_program_info_->closesthit_shader,
            shaderc_closesthit_shader);
    }
    catch (const std::exception& ex)
    {
        logger_->warn(
            "Failed to compile Vulkan ray tracing shaders ({}, {}, {}): {}",
            active_program_info_->raygen_shader.string(),
            active_program_info_->miss_shader.string(),
            active_program_info_->closesthit_shader.string(),
            ex.what());
        return;
    }

    auto raygen_module = CreateShaderModule(raygen_code);
    auto miss_module = CreateShaderModule(miss_code);
    auto closesthit_module = CreateShaderModule(closesthit_code);

    std::array<vk::PipelineShaderStageCreateInfo, 3> shader_stages = {{
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eRaygenKHR,
         *raygen_module,
         "main"},
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eMissKHR,
         *miss_module,
         "main"},
        {vk::PipelineShaderStageCreateFlags{},
         vk::ShaderStageFlagBits::eClosestHitKHR,
         *closesthit_module,
         "main"},
    }};

    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> shader_groups = {{
        {vk::RayTracingShaderGroupTypeKHR::eGeneral,
         0,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
        {vk::RayTracingShaderGroupTypeKHR::eGeneral,
         1,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
        {vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
         VK_SHADER_UNUSED_KHR,
         2,
         VK_SHADER_UNUSED_KHR,
         VK_SHADER_UNUSED_KHR},
    }};

    const vk::DescriptorSetLayout layouts[] = {*descriptor_set_layout_};
    vk::PipelineLayoutCreateInfo layout_info(
        vk::PipelineLayoutCreateFlags{},
        1,
        layouts);
    raytracing_pipeline_layout_ =
        vk_unique_device_->createPipelineLayoutUnique(layout_info);

    vk::RayTracingPipelineCreateInfoKHR pipeline_info{};
    pipeline_info.setStages(shader_stages);
    pipeline_info.setGroups(shader_groups);
    pipeline_info.setMaxPipelineRayRecursionDepth(1);
    pipeline_info.setLayout(*raytracing_pipeline_layout_);
    auto pipeline_result = vk_unique_device_->createRayTracingPipelineKHRUnique(
        vk::DeferredOperationKHR{},
        vk::PipelineCache{},
        pipeline_info);
    if (pipeline_result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to create Vulkan ray tracing pipeline.");
    }
    raytracing_pipeline_ = std::move(pipeline_result.value);

    const std::uint32_t handle_size =
        raytracing_pipeline_properties_.shaderGroupHandleSize;
    const std::uint32_t handle_alignment =
        raytracing_pipeline_properties_.shaderGroupHandleAlignment;
    const std::uint32_t base_alignment =
        raytracing_pipeline_properties_.shaderGroupBaseAlignment;
    const std::uint32_t handle_size_aligned =
        AlignUp(handle_size, handle_alignment);
    const std::uint32_t raygen_stride =
        AlignUp(handle_size_aligned, base_alignment);
    const std::uint32_t miss_stride = handle_size_aligned;
    const std::uint32_t hit_stride = handle_size_aligned;
    const std::uint32_t raygen_size = raygen_stride;
    const std::uint32_t miss_size =
        AlignUp(handle_size_aligned, base_alignment);
    const std::uint32_t hit_size =
        AlignUp(handle_size_aligned, base_alignment);
    const vk::DeviceSize sbt_size =
        static_cast<vk::DeviceSize>(raygen_size + miss_size + hit_size);

    std::vector<std::uint8_t> shader_handles(
        static_cast<std::size_t>(handle_size * shader_groups.size()));
    const vk::Result handle_result =
        vk_unique_device_->getRayTracingShaderGroupHandlesKHR(
        *raytracing_pipeline_,
        0,
        static_cast<std::uint32_t>(shader_groups.size()),
        shader_handles.size(),
        shader_handles.data());
    if (handle_result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to query Vulkan ray tracing shader group handles.");
    }

    raytracing_sbt_buffer_ = gpu_memory_manager_->CreateBuffer(
        sbt_size,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        raytracing_sbt_memory_,
        vk::MemoryAllocateFlagBits::eDeviceAddress);
    auto* mapped = static_cast<std::uint8_t*>(vk_unique_device_->mapMemory(
        *raytracing_sbt_memory_,
        0,
        sbt_size));
    std::memset(mapped, 0, static_cast<std::size_t>(sbt_size));
    std::memcpy(mapped, shader_handles.data(), handle_size);
    std::memcpy(
        mapped + raygen_size,
        shader_handles.data() + handle_size,
        handle_size);
    std::memcpy(
        mapped + raygen_size + miss_size,
        shader_handles.data() + handle_size * 2,
        handle_size);
    vk_unique_device_->unmapMemory(*raytracing_sbt_memory_);

    const vk::DeviceAddress sbt_address = vk_unique_device_->getBufferAddress(
        vk::BufferDeviceAddressInfo(*raytracing_sbt_buffer_));
    raygen_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address,
        raygen_stride,
        raygen_size);
    miss_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address + raygen_size,
        miss_stride,
        miss_size);
    hit_sbt_region_ = vk::StridedDeviceAddressRegionKHR(
        sbt_address + raygen_size + miss_size,
        hit_stride,
        hit_size);
    callable_sbt_region_ = vk::StridedDeviceAddressRegionKHR();

    logger_->info("Created Vulkan ray tracing pipeline.");
}

void Device::DestroyRaytracingPipeline()
{
    raytracing_pipeline_.reset();
    raytracing_pipeline_layout_.reset();
    raytracing_sbt_buffer_.reset();
    raytracing_sbt_memory_.reset();
    raygen_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    miss_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    hit_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    callable_sbt_region_ = vk::StridedDeviceAddressRegionKHR();
    compute_output_in_shader_read_ = false;
}

vk::UniqueShaderModule Device::CreateShaderModule(
    const std::vector<std::uint32_t>& code) const
{
    vk::ShaderModuleCreateInfo create_info(
        vk::ShaderModuleCreateFlags{},
        code.size() * sizeof(std::uint32_t),
        code.data());
    return vk_unique_device_->createShaderModuleUnique(create_info);
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
    instances.reserve(2);

    const auto append_geometry =
        [&](const char* inner_name,
            std::uint32_t instance_custom_index) {
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
            geometry.instance_custom_index = instance_custom_index;

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
                vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
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

    append_geometry("TriangleBufferTransmissive", 0u);
    append_geometry("TriangleBufferOpaque", 1u);

    if (instances.empty())
    {
        use_hardware_raytracing_ = false;
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
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
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
    hardware_raytracing_geometries_.clear();
}

void Device::CreateComputeOutputImage()
{
    if (!use_compute_raytracing_ || !vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid())
    {
        return;
    }
    const auto extent2d = swapchain_resources_->GetExtent();
    if (extent2d.width == 0 || extent2d.height == 0)
    {
        return;
    }

    const auto format_props =
        vk_physical_device_.getFormatProperties(compute_output_format_);
    if (!(format_props.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eStorageImage))
    {
        throw std::runtime_error(
            "Compute output format lacks storage image support on this device.");
    }
    if (compute_output_format_ == vk::Format::eR16G16B16A16Sfloat &&
        !vk_physical_device_.getFeatures().shaderStorageImageExtendedFormats)
    {
        throw std::runtime_error(
            "shaderStorageImageExtendedFormats is required for rgba16f compute output.");
    }

    vk::Extent3D extent{
        extent2d.width,
        extent2d.height,
        1};

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        compute_output_format_,
        extent,
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    compute_output_image_ = vk_unique_device_->createImageUnique(image_info);
    auto requirements =
        vk_unique_device_->getImageMemoryRequirements(*compute_output_image_);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    compute_output_memory_ =
        vk_unique_device_->allocateMemoryUnique(allocate_info);
    vk_unique_device_->bindImageMemory(
        *compute_output_image_, *compute_output_memory_, 0);

    TransitionImageLayout(
        *compute_output_image_,
        compute_output_format_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *compute_output_image_,
        vk::ImageViewType::e2D,
        compute_output_format_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    compute_output_view_ =
        vk_unique_device_->createImageViewUnique(view_info);

    if (!compute_output_sampler_)
    {
        vk::SamplerCreateInfo sampler_info(
            vk::SamplerCreateFlags{},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            0.0f,
            VK_FALSE,
            1.0f,
            VK_FALSE,
            vk::CompareOp::eAlways,
            0.0f,
            0.0f,
            vk::BorderColor::eIntOpaqueBlack,
            VK_FALSE);
        compute_output_sampler_ =
            vk_unique_device_->createSamplerUnique(sampler_info);
    }
    compute_output_in_shader_read_ = false;
}

void Device::DestroyComputeOutputImage()
{
    compute_output_sampler_.reset();
    compute_output_view_.reset();
    compute_output_image_.reset();
    compute_output_memory_.reset();
    compute_output_in_shader_read_ = false;
}

void Device::CreateSwapchainPreviewImage()
{
    if (!vk_unique_device_ || !swapchain_resources_ ||
        !swapchain_resources_->IsValid() || !gpu_memory_manager_)
    {
        return;
    }

    const auto extent = swapchain_resources_->GetExtent();
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    const vk::Format swapchain_format = swapchain_resources_->GetImageFormat();
    const glm::uvec2 swapchain_size = {extent.width, extent.height};

    const bool has_valid_preview_resources =
        static_cast<bool>(swapchain_preview_image_) &&
        static_cast<bool>(swapchain_preview_view_) &&
        static_cast<bool>(swapchain_preview_sampler_);
    if (has_valid_preview_resources &&
        swapchain_preview_format_ == swapchain_format &&
        swapchain_preview_size_ == swapchain_size)
    {
        // Keep the existing preview image alive across level rebuilds.
        return;
    }

    DestroySwapchainPreviewImage();
    swapchain_preview_format_ = swapchain_format;
    swapchain_preview_size_ = swapchain_size;

    const auto format_props =
        vk_physical_device_.getFormatProperties(swapchain_preview_format_);
    if (!(format_props.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImage))
    {
        logger_->warn(
            "Swapchain format {} cannot be sampled; windowed Vulkan preview disabled.",
            vk::to_string(swapchain_preview_format_));
        return;
    }

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        swapchain_preview_format_,
        vk::Extent3D(extent.width, extent.height, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled);
    swapchain_preview_image_ = vk_unique_device_->createImageUnique(image_info);

    auto requirements =
        vk_unique_device_->getImageMemoryRequirements(*swapchain_preview_image_);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    swapchain_preview_memory_ =
        vk_unique_device_->allocateMemoryUnique(allocate_info);
    vk_unique_device_->bindImageMemory(
        *swapchain_preview_image_, *swapchain_preview_memory_, 0);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *swapchain_preview_image_,
        vk::ImageViewType::e2D,
        swapchain_preview_format_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    swapchain_preview_view_ =
        vk_unique_device_->createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntOpaqueBlack,
        VK_FALSE);
    swapchain_preview_sampler_ =
        vk_unique_device_->createSamplerUnique(sampler_info);
    swapchain_preview_in_shader_read_ = false;
}

void Device::DestroySwapchainPreviewImage()
{
    swapchain_preview_sampler_.reset();
    swapchain_preview_view_.reset();
    swapchain_preview_image_.reset();
    swapchain_preview_memory_.reset();
    swapchain_preview_in_shader_read_ = false;
    swapchain_preview_format_ = vk::Format::eUndefined;
    swapchain_preview_size_ = {0, 0};
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
        if (!compute_output_view_)
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
            *compute_output_view_,
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
            *compute_output_sampler_,
            *compute_output_view_,
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




