#include "frame/vulkan/json/parse_scene_tree.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
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
#include "frame/json/program_catalog.h"
#include "frame/json/parse_uniform.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/file/file_system.h"
#include "frame/bvh.h"
#include "frame/vulkan/buffer.h"
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

bool IsRaytracingBvhMaterial(LevelInterface& level, EntityId material_id)
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
    return frame::json::IsRaytracingBvhProgramKey(key);
}

void ReplaceTextureBindingByInnerName(
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
    material.AddTextureId(texture_id, inner_name);
}

void AdoptGltfPbrTextures(
    LevelInterface& level,
    EntityId source_material_id,
    EntityId target_material_id)
{
    if (!source_material_id ||
        !target_material_id ||
        source_material_id == target_material_id)
    {
        return;
    }
    if (!IsRaytracingBvhMaterial(level, target_material_id))
    {
        return;
    }
    auto& source = level.GetMaterialFromId(source_material_id);
    auto& target = level.GetMaterialFromId(target_material_id);
    const auto find_source_texture = [&](const std::string& inner_name) {
        for (const auto texture_id : source.GetTextureIds())
        {
            if (source.GetInnerName(texture_id) == inner_name)
            {
                return texture_id;
            }
        }
        return NullId;
    };
    const auto find_target_texture = [&](const std::string& inner_name) {
        for (const auto texture_id : target.GetTextureIds())
        {
            if (target.GetInnerName(texture_id) == inner_name)
            {
                return texture_id;
            }
        }
        return NullId;
    };

    std::array<std::pair<const char*, const char*>, 5> pbr_mappings = {{
        {"albedo_texture", "Color"},
        {"normal_texture", nullptr},
        {"roughness_texture", nullptr},
        {"metallic_texture", nullptr},
        {"ao_texture", nullptr},
    }};
    for (const auto& [target_name, fallback_name] : pbr_mappings)
    {
        EntityId source_id = find_source_texture(target_name);
        if (!source_id && fallback_name)
        {
            source_id = find_source_texture(fallback_name);
        }
        if (!source_id)
        {
            continue;
        }
        const auto source_texture_name = level.GetNameFromId(source_id);
        // Only adopt file-backed glTF textures so explicit level textures are
        // not overwritten by generated solid-color fallbacks.
        if (source_texture_name.find(".__gltf_tex_") == std::string::npos)
        {
            continue;
        }

        const EntityId existing_target_id =
            find_target_texture(target_name);
        if (existing_target_id != NullId)
        {
            const auto existing_target_texture_name =
                level.GetNameFromId(existing_target_id);
            const bool replace_generated_target =
                existing_target_texture_name.find(".__gltf_solid_") !=
                    std::string::npos ||
                existing_target_texture_name.find(".__gltf_tex_") !=
                    std::string::npos;
            if (!replace_generated_target)
            {
                continue;
            }
        }
        ReplaceTextureBindingByInnerName(target, target_name, source_id);
    }
}

void EnsureRaytracingBvhBuffers(
    LevelInterface& level, EntityId material_id, const MeshInterface& mesh)
{
    if (!IsRaytracingBvhMaterial(level, material_id))
    {
        return;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto triangle_buffer_id = mesh.GetTriangleBufferId();
    if (!triangle_buffer_id)
    {
        return;
    }
    material.AddBufferName(
        level.GetNameFromId(triangle_buffer_id),
        "TriangleBuffer");

    const auto bvh_buffer_id = mesh.GetBvhBufferId();
    if (!bvh_buffer_id)
    {
        Logger::GetInstance()->warn(
            "Raytracing material '{}' has no BVH buffer bound.",
            material.GetData().name());
        return;
    }
    material.AddBufferName(level.GetNameFromId(bvh_buffer_id), "BvhBuffer");
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
        if (!frame::json::IsRaytracingBvhProgramKey(key))
        {
            continue;
        }
        if (scene_type == frame::proto::SceneType::SCENE &&
            !raytracing_bvh_scene_program_id)
        {
            raytracing_bvh_scene_program_id = program_id;
        }
        if (scene_type == frame::proto::SceneType::QUAD &&
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
    const auto local = (model_path.parent_path() / parsed).lexically_normal();
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
    std::unique_ptr<frame::NodeMatrix> node_matrix;
    const bool rotation =
        proto_matrix.matrix_type_enum() ==
            frame::proto::NodeMatrix::ROTATION_MATRIX ||
        proto_matrix.has_quaternion();

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

    auto material_id = level.GetIdFromName(proto_mesh.material_name());
    if (!material_id)
    {
        throw std::runtime_error(std::format(
            "Material {} not found for static mesh {}.",
            proto_mesh.material_name(),
            proto_mesh.name()));
    }

    auto node = std::make_unique<frame::NodeMesh>(
        MakeResolver(level), mesh_id);
    node->GetData().set_name(proto_mesh.name());
    node->SetParentName(proto_mesh.parent());
    node->GetData().set_material_name(proto_mesh.material_name());
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
    const frame::proto::NodeMesh& proto_mesh)
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
        const EntityId explicit_material_id =
            proto_mesh.material_name().empty()
                ? NullId
                : level.GetIdFromName(proto_mesh.material_name());
        if (!proto_mesh.material_name().empty() && !explicit_material_id)
        {
            Logger::GetInstance()->warn(
                "Material '{}' was not found for mesh '{}'. Falling back to glTF material.",
                proto_mesh.material_name(),
                proto_mesh.name());
        }
        if (explicit_material_id)
        {
            ConfigureMaterialProgramsForRenderTime(
                level,
                explicit_material_id,
                proto_mesh.render_time_enum());
        }
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
        const bool selected_program_is_raytracing_bvh =
            selected_program &&
            frame::json::IsRaytracingBvhProgramKey(
                frame::json::ResolveProgramKey(selected_program->GetData()));
        const bool build_bvh =
            selected_program_is_raytracing_bvh ||
            proto_mesh.acceleration_structure_enum() ==
                frame::proto::NodeMesh::BVH_ACCELERATION;
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
            frame::proto::Texture proto_texture;
            const std::string texture_name = std::format(
                "{}.__gltf_solid_{}_{}",
                proto_mesh.name(),
                semantic,
                generated_texture_counter++);
            proto_texture.set_name(texture_name);
            proto_texture.mutable_pixel_element_size()->CopyFrom(
                frame::json::PixelElementSize_BYTE());
            proto_texture.mutable_pixel_structure()->CopyFrom(
                frame::json::PixelStructure_RGB_ALPHA());
            proto_texture.mutable_size()->set_x(1);
            proto_texture.mutable_size()->set_y(1);
            std::array<std::uint8_t, 4> pixels = {
                static_cast<std::uint8_t>(std::clamp(color.x, 0.0f, 1.0f) * 255.0f),
                static_cast<std::uint8_t>(std::clamp(color.y, 0.0f, 1.0f) * 255.0f),
                static_cast<std::uint8_t>(std::clamp(color.z, 0.0f, 1.0f) * 255.0f),
                static_cast<std::uint8_t>(std::clamp(color.w, 0.0f, 1.0f) * 255.0f)};
            proto_texture.set_pixels(
                reinterpret_cast<const char*>(pixels.data()), pixels.size());
            auto texture =
                frame::vulkan::json::ParseTexture(
                    proto_texture, texture_display_size);
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
                ai_material, path, {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE});
            const auto normal_texture_path = FindFirstMaterialTexturePath(
                ai_material, path, {aiTextureType_NORMALS, aiTextureType_HEIGHT});
            const auto roughness_texture_path = FindFirstMaterialTexturePath(
                ai_material, path, {aiTextureType_DIFFUSE_ROUGHNESS});
            const auto metallic_texture_path = FindFirstMaterialTexturePath(
                ai_material, path, {aiTextureType_METALNESS});
            const auto ao_texture_path = FindFirstMaterialTexturePath(
                ai_material, path, {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP});

            const glm::vec4 base_color_factor = ReadBaseColorFactor(ai_material);
            const float roughness_factor = ReadRoughnessFactor(ai_material);
            const float metallic_factor = ReadMetallicFactor(ai_material);

            const EntityId base_texture_id = base_color_texture_path
                ? create_texture_from_path(*base_color_texture_path, "base_color")
                : create_solid_texture(base_color_factor, "base_color");
            const EntityId normal_texture_id = normal_texture_path
                ? create_texture_from_path(*normal_texture_path, "normal")
                : create_solid_texture(
                      glm::vec4(0.5f, 0.5f, 1.0f, 1.0f), "normal");
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

            auto material = std::make_unique<frame::vulkan::Material>();
            const std::string material_name = std::format(
                "{}.__gltf_material_{}",
                proto_mesh.name(),
                generated_material_counter++);
            material->SetName(material_name);
            material->SetSerializeEnable(true);
            material->SetProgramId(selected_program_id);

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

            const EntityId material_id = level.AddMaterial(std::move(material));
            if (cache_material)
            {
                gltf_material_cache.emplace(material_index, material_id);
            }
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
            if (explicit_material_id && gltf_material_id)
            {
                AdoptGltfPbrTextures(
                    level, gltf_material_id, explicit_material_id);
            }
            const EntityId material_id =
                explicit_material_id ? explicit_material_id : gltf_material_id;
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
                    textures.push_back(0.5f);
                    textures.push_back(0.5f);
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
            EnsureRaytracingBvhBuffers(
                level,
                material_id,
                level.GetMeshFromId(mesh_id));

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
            if (material_id != NullId)
            {
                node->GetData().set_material_name(
                    level.GetNameFromId(material_id));
            }
            else
            {
                node->GetData().clear_material_name();
            }
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
    LevelInterface& level)
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
        if (!ParseNodeMesh(level, node_mesh))
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

    return true;
}

} // namespace frame::vulkan::json


