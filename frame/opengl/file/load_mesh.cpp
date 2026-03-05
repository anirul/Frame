#include "frame/opengl/file/load_mesh.h"

#include <format>
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <stdexcept>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "frame/file/file_system.h"
#include "frame/logger.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/buffer.h"
#include "frame/bvh.h"
#include "frame/opengl/json/parse_texture.h"
#include "frame/json/program_catalog.h"
#include "frame/json/parse_pixel.h"
#include "frame/opengl/material.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/program.h"

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

bool IsRaytracingBvhProgram(const ProgramInterface* program)
{
    if (!program)
    {
        return false;
    }
    const auto key = frame::json::ResolveProgramKey(program->GetData());
    return frame::json::IsRaytracingBvhProgramKey(key);
}

EntityId SelectGltfProgramId(LevelInterface& level)
{
    EntityId first_program_id = NullId;
    EntityId first_scene_program_id = NullId;
    EntityId first_quad_program_id = NullId;
    EntityId raytracing_bvh_scene_program_id = NullId;
    EntityId raytracing_bvh_quad_program_id = NullId;
    for (const auto program_id : level.GetPrograms())
    {
        if (!first_program_id)
        {
            first_program_id = program_id;
        }
        const auto& program = level.GetProgramFromId(program_id);
        const auto scene_type = program.GetData().input_scene_type().value();
        if (scene_type == proto::SceneType::SCENE && !first_scene_program_id)
        {
            first_scene_program_id = program_id;
        }
        if (scene_type == proto::SceneType::QUAD && !first_quad_program_id)
        {
            first_quad_program_id = program_id;
        }
        const auto key = frame::json::ResolveProgramKey(program.GetData());
        if (!frame::json::IsRaytracingBvhProgramKey(key))
        {
            continue;
        }
        if (scene_type == proto::SceneType::SCENE &&
            !raytracing_bvh_scene_program_id)
        {
            raytracing_bvh_scene_program_id = program_id;
        }
        if (scene_type == proto::SceneType::QUAD &&
            !raytracing_bvh_quad_program_id)
        {
            raytracing_bvh_quad_program_id = program_id;
        }
    }
    if (raytracing_bvh_quad_program_id)
    {
        return raytracing_bvh_quad_program_id;
    }
    if (raytracing_bvh_scene_program_id)
    {
        return raytracing_bvh_scene_program_id;
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

std::optional<std::filesystem::path> ResolveMaterialTexturePath(
    const std::filesystem::path& model_path,
    const aiString& texture_path)
{
    const std::string raw = texture_path.C_Str();
    if (raw.empty() || raw.front() == '*')
    {
        return std::nullopt;
    }
    const std::filesystem::path parsed(raw);
    if (parsed.is_absolute() && std::filesystem::exists(parsed))
    {
        return parsed.lexically_normal();
    }
    const auto local =
        (model_path.parent_path() / parsed).lexically_normal();
    if (std::filesystem::exists(local))
    {
        return local;
    }
    try
    {
        return frame::file::FindFile(parsed).lexically_normal();
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<std::filesystem::path> FindFirstMaterialTexturePath(
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
        auto resolved = ResolveMaterialTexturePath(model_path, texture_path);
        if (resolved)
        {
            return resolved;
        }
    }
    return std::nullopt;
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

struct Bounds3
{
    aiVector3D min = aiVector3D(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());
    aiVector3D max = aiVector3D(
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest());
    bool valid = false;

    void Expand(const aiVector3D& p)
    {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
        valid = true;
    }

    aiVector3D Center() const
    {
        return aiVector3D(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f);
    }
};

glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
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
        BuildNodeHierarchy(
            node->mChildren[child],
            node_index,
            nodes,
            node_indices);
    }
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

const AnimationClipData* SelectAnimationClip(
    const SkinAnimationData& animation_data,
    const std::string& clip_name,
    const std::optional<std::uint32_t>& clip_index,
    std::size_t* selected_index = nullptr)
{
    if (animation_data.clips.empty())
    {
        return nullptr;
    }
    if (!clip_name.empty())
    {
        const auto name_key = ToLowerAscii(clip_name);
        auto it = animation_data.clip_name_to_index.find(name_key);
        if (it != animation_data.clip_name_to_index.end() &&
            it->second < animation_data.clips.size())
        {
            if (selected_index)
            {
                *selected_index = it->second;
            }
            return &animation_data.clips[it->second];
        }
    }
    if (clip_index && *clip_index < animation_data.clips.size())
    {
        if (selected_index)
        {
            *selected_index = *clip_index;
        }
        return &animation_data.clips[*clip_index];
    }
    if (selected_index)
    {
        *selected_index = 0;
    }
    return &animation_data.clips[0];
}

std::string ClipDisplayName(const AnimationClipData& clip, std::size_t index)
{
    if (!clip.name.empty())
    {
        return clip.name;
    }
    return std::format("<unnamed:{}>", index);
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
        auto channel_it = clip_data->channels.find(node_index);
        if (channel_it != clip_data->channels.end())
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
                                           : InterpolateVectorKeys(
                                                 channel.position_keys,
                                                 time_ticks);
        const aiQuaternion rotation = channel.rotation_keys.empty()
                                          ? bind_rotation
                                          : InterpolateQuatKeys(
                                                channel.rotation_keys,
                                                time_ticks);
        const aiVector3D scaling = channel.scaling_keys.empty()
                                       ? bind_scaling
                                       : InterpolateVectorKeys(
                                             channel.scaling_keys,
                                             time_ticks);
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
    std::optional<std::uint32_t> clip_index)
{
    if (!animation_data || animation_data->bones.empty() || animation_data->nodes.empty())
    {
        return {};
    }
    const AnimationClipData* clip_data = SelectAnimationClip(
        *animation_data,
        clip_name,
        clip_index);

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

    std::vector<aiMatrix4x4> node_globals(
        animation_data->nodes.size(), aiMatrix4x4());
    EvaluateNodeTransformsRecursive(
        *animation_data,
        0,
        aiMatrix4x4(),
        time_ticks,
        node_globals,
        clip_data);

    std::vector<glm::mat4> bone_matrices(
        animation_data->bones.size(), glm::mat4(1.0f));
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

std::vector<float> BuildRaytraceTriangles(
    const std::vector<float>& points,
    const std::vector<float>& normals,
    const std::vector<float>& textures,
    const std::vector<std::uint32_t>& indices)
{
    std::vector<float> triangles;
    triangles.reserve(indices.size() * 16);
    auto push_vertex = [&](std::uint32_t index) {
        const std::size_t point_offset = static_cast<std::size_t>(index) * 3;
        if (point_offset + 2 >= points.size())
        {
            return;
        }
        triangles.push_back(points[point_offset]);
        triangles.push_back(points[point_offset + 1]);
        triangles.push_back(points[point_offset + 2]);
        triangles.push_back(0.0f);

        if (point_offset + 2 < normals.size())
        {
            triangles.push_back(normals[point_offset]);
            triangles.push_back(normals[point_offset + 1]);
            triangles.push_back(normals[point_offset + 2]);
        }
        else
        {
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
            triangles.push_back(0.0f);
        }
        triangles.push_back(0.0f);

        const std::size_t texture_offset = static_cast<std::size_t>(index) * 2;
        if (texture_offset + 1 < textures.size())
        {
            triangles.push_back(textures[texture_offset]);
            triangles.push_back(textures[texture_offset + 1]);
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
    if (!skin_animation_data || vertex_count == 0 ||
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
                points[offset], points[offset + 1], points[offset + 2], 1.0f);
        skinned_points[offset] = point.x;
        skinned_points[offset + 1] = point.y;
        skinned_points[offset + 2] = point.z;

        if (offset + 2 < normals.size())
        {
            glm::vec3 normal = glm::mat3(skin_matrix) *
                               glm::vec3(
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

std::vector<std::pair<EntityId, EntityId>> LoadMeshesFromGltfFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name,
    proto::NodeMesh::AccelerationStructureEnum acceleration_structure_enum,
    EntityId forced_program_id)
{
    std::vector<std::pair<EntityId, EntityId>> entity_id_vec;
    (void)material_name;

    Assimp::Importer importer;
    constexpr unsigned int kPostProcessFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(file.string(), kPostProcessFlags);
    if (!scene || !scene->HasMeshes())
    {
        Logger::GetInstance()->error(
            "Failed to load glTF file {}: {}",
            file.string(),
            importer.GetErrorString());
        return {};
    }

    std::unordered_map<unsigned int, aiMatrix4x4> mesh_transforms;
    if (scene->mRootNode)
    {
        GatherNodeMeshTransforms(scene->mRootNode, aiMatrix4x4(), mesh_transforms);
    }

    const EntityId selected_program_id =
        forced_program_id ? forced_program_id : SelectGltfProgramId(level);
    ProgramInterface* selected_program =
        (selected_program_id != NullId)
            ? &level.GetProgramFromId(selected_program_id)
            : nullptr;
    const bool selected_program_is_raytracing_bvh =
        IsRaytracingBvhProgram(selected_program);
    const glm::uvec2 texture_display_size = ResolveTextureDisplaySize(level);
    std::unordered_map<std::string, EntityId> file_texture_cache = {};
    std::unordered_map<std::string, EntityId> solid_texture_cache = {};
    std::unordered_map<unsigned int, EntityId> gltf_material_cache = {};
    int generated_texture_counter = 0;
    int generated_material_counter = 0;

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
        proto::Texture proto_texture;
        const std::string texture_name = std::format(
            "{}.__gltf_tex_{}_{}",
            name,
            semantic,
            generated_texture_counter++);
        proto_texture.set_name(texture_name);
        proto_texture.mutable_pixel_element_size()->CopyFrom(
            frame::json::PixelElementSize_BYTE());
        proto_texture.mutable_pixel_structure()->CopyFrom(
            frame::json::PixelStructure_RGB_ALPHA());
        proto_texture.set_file_name(frame::file::PurifyFilePath(normalized));
        auto texture =
            frame::json::ParseTexture(proto_texture, texture_display_size);
        texture->SetName(texture_name);
        texture->SetSerializeEnable(true);
        EntityId texture_id = level.AddTexture(std::move(texture));
        file_texture_cache.emplace(cache_key, texture_id);
        return texture_id;
    };

    auto create_solid_texture = [&](const glm::vec4& color,
                                    const std::string& semantic) -> EntityId {
        const std::string cache_key = std::format(
            "{}|{}|{}|{}|{}",
            semantic,
            color.x,
            color.y,
            color.z,
            color.w);
        if (auto it = solid_texture_cache.find(cache_key);
            it != solid_texture_cache.end())
        {
            return it->second;
        }
        auto texture = frame::opengl::file::LoadTextureFromVec4(color);
        const std::string texture_name = std::format(
            "{}.__gltf_solid_{}_{}",
            name,
            semantic,
            generated_texture_counter++);
        texture->SetName(texture_name);
        texture->SetSerializeEnable(true);
        EntityId texture_id = level.AddTexture(std::move(texture));
        solid_texture_cache.emplace(cache_key, texture_id);
        return texture_id;
    };

    auto create_material_from_gltf = [&](unsigned int material_index) -> EntityId {
        if (selected_program_id == NullId ||
            material_index >= scene->mNumMaterials)
        {
            return NullId;
        }
        const bool cache_material = !selected_program_is_raytracing_bvh;
        if (cache_material)
        {
            if (auto it = gltf_material_cache.find(material_index);
                it != gltf_material_cache.end())
            {
                return it->second;
            }
        }
        const aiMaterial* ai_material = scene->mMaterials[material_index];
        const auto base_color_texture_path = FindFirstMaterialTexturePath(
            ai_material, file, {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE});
        const auto normal_texture_path = FindFirstMaterialTexturePath(
            ai_material, file, {aiTextureType_NORMALS, aiTextureType_HEIGHT});
        const auto roughness_texture_path = FindFirstMaterialTexturePath(
            ai_material, file, {aiTextureType_DIFFUSE_ROUGHNESS});
        const auto metallic_texture_path = FindFirstMaterialTexturePath(
            ai_material, file, {aiTextureType_METALNESS});
        const auto ao_texture_path = FindFirstMaterialTexturePath(
            ai_material, file, {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP});

        const glm::vec4 base_color_factor = ReadBaseColorFactor(ai_material);
        const float roughness_factor = ReadRoughnessFactor(ai_material);
        const float metallic_factor = ReadMetallicFactor(ai_material);

        const EntityId base_texture_id = base_color_texture_path
            ? create_texture_from_path(*base_color_texture_path, "base_color")
            : create_solid_texture(base_color_factor, "base_color");
        const EntityId normal_texture_id = normal_texture_path
            ? create_texture_from_path(*normal_texture_path, "normal")
            : create_solid_texture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f), "normal");
        const EntityId roughness_texture_id = roughness_texture_path
            ? create_texture_from_path(*roughness_texture_path, "roughness")
            : create_solid_texture(
                  glm::vec4(
                      roughness_factor,
                      roughness_factor,
                      roughness_factor,
                      1.0f),
                  "roughness");
        const EntityId metallic_texture_id = metallic_texture_path
            ? create_texture_from_path(*metallic_texture_path, "metallic")
            : create_solid_texture(
                  glm::vec4(
                      metallic_factor,
                      metallic_factor,
                      metallic_factor,
                      1.0f),
                  "metallic");
        const EntityId ao_texture_id = ao_texture_path
            ? create_texture_from_path(*ao_texture_path, "ao")
            : create_solid_texture(glm::vec4(1.0f), "ao");

        auto material = std::make_unique<frame::opengl::Material>();
        const std::string material_name_generated = std::format(
            "{}.__gltf_material_{}",
            name,
            generated_material_counter++);
        material->SetName(material_name_generated);
        material->SetSerializeEnable(true);
        material->SetProgramId(selected_program_id);
        if (selected_program_is_raytracing_bvh)
        {
            const auto preprocess_id =
                level.GetIdFromName("RayTracePreprocessProgram");
            if (preprocess_id != NullId)
            {
                material->SetPreprocessProgramId(preprocess_id);
            }
        }

        bool has_bound_from_bindings = false;
        if (selected_program)
        {
            for (const auto& binding : selected_program->GetData().bindings())
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
                else
                {
                    texture_id = level.GetIdFromName(binding_name);
                }
                if (texture_id == NullId)
                {
                    continue;
                }
                material->AddTextureId(texture_id, binding_name);
                has_bound_from_bindings = true;
            }
        }
        if (!has_bound_from_bindings)
        {
            if (!selected_program ||
                selected_program->HasUniform("albedo_texture"))
            {
                material->AddTextureId(base_texture_id, "albedo_texture");
            }
            else if (selected_program->HasUniform("Color"))
            {
                material->AddTextureId(base_texture_id, "Color");
            }
            if (!selected_program ||
                selected_program->HasUniform("normal_texture"))
            {
                material->AddTextureId(normal_texture_id, "normal_texture");
            }
            if (!selected_program ||
                selected_program->HasUniform("roughness_texture"))
            {
                material->AddTextureId(
                    roughness_texture_id, "roughness_texture");
            }
            if (!selected_program ||
                selected_program->HasUniform("metallic_texture"))
            {
                material->AddTextureId(metallic_texture_id, "metallic_texture");
            }
            if (!selected_program ||
                selected_program->HasUniform("ao_texture"))
            {
                material->AddTextureId(ao_texture_id, "ao_texture");
            }
            auto skybox_id = level.GetIdFromName("skybox");
            if (skybox_id && (!selected_program ||
                              selected_program->HasUniform("skybox")))
            {
                material->AddTextureId(skybox_id, "skybox");
            }
            auto skybox_env_id = level.GetIdFromName("skybox_env");
            if (skybox_env_id && (!selected_program ||
                                  selected_program->HasUniform("skybox_env")))
            {
                material->AddTextureId(skybox_env_id, "skybox_env");
            }
        }

        const EntityId material_id = level.AddMaterial(std::move(material));
        if (cache_material)
        {
            gltf_material_cache.emplace(material_index, material_id);
        }
        return material_id;
    };

    auto make_node_resolver = [&level](const std::string& node_name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(node_name);
        if (!maybe_id)
        {
            throw std::runtime_error(
                std::format("no id for name: {}", node_name));
        }
        return &level.GetSceneNodeFromId(maybe_id);
    };

    std::vector<SkinAnimationData::NodeData> scene_nodes = {};
    std::unordered_map<std::string, int> scene_node_indices = {};
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
    std::vector<AnimationClipData> scene_animation_clips = {};
    std::unordered_map<std::string, std::size_t> scene_clip_name_to_index = {};
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
        clip_data.has_animation = animation->mDuration > 0.0;
        for (unsigned int channel_index = 0;
             channel_index < animation->mNumChannels;
             ++channel_index)
        {
            const aiNodeAnim* channel = animation->mChannels[channel_index];
            auto node_it = scene_node_indices.find(channel->mNodeName.C_Str());
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
            const std::string clip_name_key = ToLowerAscii(clip_data.name);
            if (!scene_clip_name_to_index.contains(clip_name_key))
            {
                scene_clip_name_to_index[clip_name_key] = clip_index;
            }
        }
        Logger::GetInstance()->info(
            "glTF clip[{}] '{}' (duration ticks: {}, ticks/s: {}, channels: {})",
            clip_index,
            ClipDisplayName(clip_data, clip_index),
            clip_data.duration_ticks,
            clip_data.ticks_per_second,
            clip_data.channels.size());
        scene_animation_clips.push_back(std::move(clip_data));
    }
    if (!scene_animation_clips.empty())
    {
        Logger::GetInstance()->info(
            "glTF animation clips loaded: {} (default='{}').",
            scene_animation_clips.size(),
            ClipDisplayName(scene_animation_clips.front(), 0));
    }

    std::size_t skinned_mesh_count = 0;
    const bool build_bvh =
        selected_program_is_raytracing_bvh ||
        acceleration_structure_enum == proto::NodeMesh::BVH_ACCELERATION;
    Bounds3 local_model_bounds;
    Bounds3 transformed_model_bounds;
    for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
        const aiMesh* mesh = scene->mMeshes[mesh_index];
        if (!mesh || mesh->mNumVertices == 0)
        {
            continue;
        }
        EntityId material_id = NullId;
        if (mesh->mMaterialIndex < scene->mNumMaterials)
        {
            material_id = create_material_from_gltf(mesh->mMaterialIndex);
        }
        if (mesh->HasBones())
        {
            ++skinned_mesh_count;
        }

        aiMatrix4x4 mesh_transform = aiMatrix4x4();
        auto transform_it = mesh_transforms.find(mesh_index);
        if (transform_it != mesh_transforms.end())
        {
            mesh_transform = transform_it->second;
        }
        aiMatrix3x3 normal_transform(mesh_transform);
        normal_transform.Inverse().Transpose();
        Bounds3 local_mesh_bounds;
        Bounds3 transformed_mesh_bounds;

        std::vector<float> points;
        std::vector<float> normals;
        std::vector<float> textures;
        std::vector<std::uint32_t> indices;
        points.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 3);
        normals.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 3);
        textures.reserve(static_cast<std::size_t>(mesh->mNumVertices) * 2);

        const bool has_normals = mesh->HasNormals();
        const bool has_texcoords = mesh->HasTextureCoords(0);
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            const aiVector3D local_p = mesh->mVertices[v];
            local_mesh_bounds.Expand(local_p);
            local_model_bounds.Expand(local_p);
            const aiVector3D p = mesh_transform * local_p;
            transformed_mesh_bounds.Expand(p);
            transformed_model_bounds.Expand(p);
            points.push_back(p.x);
            points.push_back(p.y);
            points.push_back(p.z);

            if (has_normals)
            {
                aiVector3D n = normal_transform * mesh->mNormals[v];
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
                const auto& uv = mesh->mTextureCoords[0][v];
                textures.push_back(uv.x);
                textures.push_back(uv.y);
            }
            else
            {
                textures.push_back(0.5f);
                textures.push_back(0.5f);
            }
        }

        for (unsigned int face_index = 0; face_index < mesh->mNumFaces; ++face_index)
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
        if (indices.empty())
        {
            continue;
        }
        const aiVector3D local_center = local_mesh_bounds.Center();
        const aiVector3D transformed_center = transformed_mesh_bounds.Center();
        Logger::GetInstance()->info(
            "glTF mesh {}[{}] local bounds min({}, {}, {}) max({}, {}, {}) "
            "center({}, {}, {}), transformed center({}, {}, {})",
            name,
            mesh_index,
            local_mesh_bounds.min.x,
            local_mesh_bounds.min.y,
            local_mesh_bounds.min.z,
            local_mesh_bounds.max.x,
            local_mesh_bounds.max.y,
            local_mesh_bounds.max.z,
            local_center.x,
            local_center.y,
            local_center.z,
            transformed_center.x,
            transformed_center.y,
            transformed_center.z);

        std::shared_ptr<SkinAnimationData> skin_animation_data = nullptr;
        std::vector<int> bone_indices_flat = {};
        std::vector<float> bone_weights_flat = {};
        if (mesh->HasBones())
        {
            constexpr std::size_t kMaxBones = 128;
            std::size_t supported_bones =
                std::min<std::size_t>(mesh->mNumBones, kMaxBones);
            if (mesh->mNumBones > supported_bones)
            {
                Logger::GetInstance()->warn(
                    "glTF mesh {}[{}] has {} bones, clamping to {}.",
                    name,
                    mesh_index,
                    mesh->mNumBones,
                    supported_bones);
            }
            skin_animation_data = std::make_shared<SkinAnimationData>();
            skin_animation_data->nodes = scene_nodes;
            skin_animation_data->node_indices = scene_node_indices;
            skin_animation_data->global_inverse_transform = scene_global_inverse;
            skin_animation_data->bones.resize(supported_bones);
            skin_animation_data->clips = scene_animation_clips;
            skin_animation_data->clip_name_to_index = scene_clip_name_to_index;

            std::vector<std::array<int, 4>> vertex_bone_indices(
                mesh->mNumVertices, std::array<int, 4>{0, 0, 0, 0});
            std::vector<std::array<float, 4>> vertex_bone_weights(
                mesh->mNumVertices, std::array<float, 4>{0.f, 0.f, 0.f, 0.f});
            for (std::size_t bone_index = 0; bone_index < supported_bones; ++bone_index)
            {
                const aiBone* bone = mesh->mBones[bone_index];
                auto node_it = skin_animation_data->node_indices.find(
                    bone->mName.C_Str());
                if (node_it == skin_animation_data->node_indices.end())
                {
                    Logger::GetInstance()->warn(
                        "Bone {} has no matching node in hierarchy.",
                        bone->mName.C_Str());
                    skin_animation_data->bones[bone_index].node_index = -1;
                }
                else
                {
                    skin_animation_data->bones[bone_index].node_index =
                        node_it->second;
                }
                skin_animation_data->bones[bone_index].offset_matrix =
                    bone->mOffsetMatrix;

                for (unsigned int weight_index = 0;
                     weight_index < bone->mNumWeights;
                     ++weight_index)
                {
                    const aiVertexWeight& weight = bone->mWeights[weight_index];
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
            for (unsigned int vertex_index = 0; vertex_index < mesh->mNumVertices; ++vertex_index)
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

        auto maybe_point_buffer_id = CreateBufferInLevel(
            level,
            points,
            std::format("{}.{}.point", name, mesh_index));
        if (!maybe_point_buffer_id)
        {
            return {};
        }
        EntityId point_buffer_id = maybe_point_buffer_id.value();

        auto maybe_normal_buffer_id = CreateBufferInLevel(
            level,
            normals,
            std::format("{}.{}.normal", name, mesh_index));
        if (!maybe_normal_buffer_id)
        {
            return {};
        }
        EntityId normal_buffer_id = maybe_normal_buffer_id.value();

        auto maybe_tex_coord_buffer_id = CreateBufferInLevel(
            level,
            textures,
            std::format("{}.{}.texture", name, mesh_index));
        if (!maybe_tex_coord_buffer_id)
        {
            return {};
        }
        EntityId tex_coord_buffer_id = maybe_tex_coord_buffer_id.value();

        auto maybe_index_buffer_id = CreateBufferInLevel(
            level,
            indices,
            std::format("{}.{}.index", name, mesh_index),
            opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
        if (!maybe_index_buffer_id)
        {
            return {};
        }
        EntityId index_buffer_id = maybe_index_buffer_id.value();

        // Triangle and optional BVH buffers for raytracing shaders.
        std::vector<std::uint32_t> trace_indices(indices.begin(), indices.end());
        auto triangles =
            BuildRaytraceTriangles(points, normals, textures, trace_indices);
        auto maybe_triangle_buffer_id = CreateBufferInLevel(
            level,
            triangles,
            std::format("{}.{}.triangle", name, mesh_index),
            opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER);
        if (!maybe_triangle_buffer_id)
        {
            return {};
        }
        EntityId triangle_buffer_id = maybe_triangle_buffer_id.value();

        EntityId bvh_buffer_id = NullId;
        if (build_bvh)
        {
            auto bvh_nodes = frame::BuildBVH(points, trace_indices);
            auto maybe_bvh_buffer_id = CreateBufferInLevel(
                level,
                bvh_nodes,
                std::format("{}.{}.bvh", name, mesh_index),
                opengl::BufferTypeEnum::SHADER_STORAGE_BUFFER);
            if (!maybe_bvh_buffer_id)
            {
                return {};
            }
            bvh_buffer_id = maybe_bvh_buffer_id.value();
        }

        MeshParameter parameter = {};
        parameter.point_buffer_id = point_buffer_id;
        parameter.normal_buffer_id = normal_buffer_id;
        parameter.texture_buffer_id = tex_coord_buffer_id;
        parameter.index_buffer_id = index_buffer_id;
        parameter.triangle_buffer_id = triangle_buffer_id;
        parameter.bvh_buffer_id = bvh_buffer_id;

        std::unique_ptr<MeshInterface> mesh_interface = nullptr;
        opengl::SkinnedMesh* skinned_mesh = nullptr;
        if (!bone_indices_flat.empty() && !bone_weights_flat.empty() &&
            skin_animation_data)
        {
            auto skinned = std::make_unique<opengl::SkinnedMesh>(
                level, parameter);
            skinned_mesh = skinned.get();
            mesh_interface = std::move(skinned);
        }
        else
        {
            mesh_interface = std::make_unique<opengl::Mesh>(
                level, parameter);
        }

        if (skinned_mesh)
        {
            auto maybe_bone_index_buffer_id = CreateBufferInLevel(
                level,
                bone_indices_flat,
                std::format("{}.{}.bone_index", name, mesh_index));
            if (!maybe_bone_index_buffer_id)
            {
                return {};
            }
            auto maybe_bone_weight_buffer_id = CreateBufferInLevel(
                level,
                bone_weights_flat,
                std::format("{}.{}.bone_weight", name, mesh_index));
            if (!maybe_bone_weight_buffer_id)
            {
                return {};
            }
            skinned_mesh->SetSkinningBuffers(
                maybe_bone_index_buffer_id.value(),
                maybe_bone_weight_buffer_id.value());
            auto* skinned_mesh_ptr = skinned_mesh;
            skinned_mesh->SetSkinningCallback(
                [skin_animation_data, skinned_mesh_ptr](double time_seconds) {
                    if (!skinned_mesh_ptr)
                    {
                        return std::vector<glm::mat4>{};
                    }
                    return EvaluateBoneMatrices(
                        skin_animation_data,
                        time_seconds,
                        skinned_mesh_ptr->GetSkinningAnimationClipName(),
                        skinned_mesh_ptr->GetSkinningAnimationClipIndex());
                });
            skinned_mesh->SetRaytraceTriangleCallback(
                [skin_animation_data,
                 skinned_mesh_ptr,
                 points,
                 normals,
                 textures,
                 trace_indices,
                 bone_indices_flat,
                 bone_weights_flat](double time_seconds) {
                    if (!skinned_mesh_ptr)
                    {
                        return std::vector<float>{};
                    }
                    return EvaluateSkinnedTriangles(
                        skin_animation_data,
                        time_seconds,
                        skinned_mesh_ptr->GetSkinningAnimationClipName(),
                        skinned_mesh_ptr->GetSkinningAnimationClipIndex(),
                        points,
                        normals,
                        textures,
                        trace_indices,
                        bone_indices_flat,
                        bone_weights_flat);
                });
            if (build_bvh)
            {
                skinned_mesh->SetRaytraceBvhCallback(
                    [skin_animation_data,
                     skinned_mesh_ptr,
                     points,
                     normals,
                     trace_indices,
                     bone_indices_flat,
                     bone_weights_flat](double time_seconds) {
                        if (!skinned_mesh_ptr)
                        {
                            return std::vector<frame::BVHNode>{};
                        }
                        return EvaluateSkinnedBvh(
                            skin_animation_data,
                            time_seconds,
                            skinned_mesh_ptr->GetSkinningAnimationClipName(),
                            skinned_mesh_ptr->GetSkinningAnimationClipIndex(),
                            points,
                            normals,
                            trace_indices,
                            bone_indices_flat,
                            bone_weights_flat);
                    });
            }
        }
        std::string mesh_name =
            scene->mNumMeshes == 1
                ? std::format("{}.mesh", name)
                : std::format("{}.{}.mesh", name, mesh_index);
        mesh_interface->SetName(mesh_name);
        auto maybe_mesh_id = level.AddMesh(std::move(mesh_interface));
        if (!maybe_mesh_id)
        {
            return {};
        }

        auto node = std::make_unique<NodeMesh>(make_node_resolver, maybe_mesh_id);
        std::string node_name =
            scene->mNumMeshes == 1
                ? name
                : std::format("{}.{}", name, mesh_index);
        node->SetName(node_name);
        auto maybe_node_id = level.AddSceneNode(std::move(node));
        if (!maybe_node_id)
        {
            return {};
        }
        entity_id_vec.push_back({maybe_node_id, material_id});
    }
    if (local_model_bounds.valid && transformed_model_bounds.valid)
    {
        const aiVector3D local_center = local_model_bounds.Center();
        const aiVector3D transformed_center = transformed_model_bounds.Center();
        Logger::GetInstance()->info(
            "glTF model {} local bounds min({}, {}, {}) max({}, {}, {}) "
            "center({}, {}, {}), transformed center({}, {}, {})",
            file.string(),
            local_model_bounds.min.x,
            local_model_bounds.min.y,
            local_model_bounds.min.z,
            local_model_bounds.max.x,
            local_model_bounds.max.y,
            local_model_bounds.max.z,
            local_center.x,
            local_center.y,
            local_center.z,
            transformed_center.x,
            transformed_center.y,
            transformed_center.z);
    }

    if (skinned_mesh_count > 0)
    {
        if (!scene_animation_clips.empty())
        {
            Logger::GetInstance()->info(
                "Loaded glTF {} with {} skinned mesh(es). "
                "Default clip is '{}' (index 0).",
                file.string(),
                skinned_mesh_count,
                ClipDisplayName(scene_animation_clips.front(), 0));
        }
        else
        {
            Logger::GetInstance()->warn(
                "Loaded glTF {} with {} skinned mesh(es) but no animation clip. "
                "Rendering bind pose.",
                file.string(),
                skinned_mesh_count);
        }
    }

    return entity_id_vec;
}

} // End namespace.

std::vector<std::pair<EntityId, EntityId>> LoadMeshesFromFile(
    LevelInterface& level_interface,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name,
    EntityId forced_program_id)
{
    return LoadMeshesFromFile(
        level_interface,
        std::move(file),
        name,
        material_name,
        proto::NodeMesh::NO_ACCELERATION,
        forced_program_id);
}

std::vector<std::pair<EntityId, EntityId>> LoadMeshesFromFile(
    LevelInterface& level_interface,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name /* = ""*/,
    proto::NodeMesh::AccelerationStructureEnum acceleration_structure_enum,
    EntityId forced_program_id)
{
    auto extension = file.extension();
    std::filesystem::path final_path = ResolveAssetPath(file);
    if (extension == ".gltf" || extension == ".glb")
    {
        return LoadMeshesFromGltfFile(
            level_interface,
            final_path,
            name,
            material_name,
            acceleration_structure_enum,
            forced_program_id);
    }
    throw std::runtime_error(
        std::format("Unknown extention for file : {}", file.string()));
}

} // End namespace frame::opengl::file.





