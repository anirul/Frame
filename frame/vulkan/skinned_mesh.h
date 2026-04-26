#pragma once

#include "frame/backend_internal.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "frame/bvh.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan
{

class SkinnedMesh : public StaticMesh
{
  public:
    SkinnedMesh(const frame::MeshParameter& parameters, bool clear_buffer)
        : StaticMesh(parameters, clear_buffer)
    {
    }
    ~SkinnedMesh() override = default;

    void SetSkinningAnimation(bool enabled, float speed = 1.0f)
    {
        skinning_animation_enabled_ = enabled;
        if (!std::isfinite(speed))
        {
            speed = 1.0f;
        }
        skinning_animation_speed_ = speed;
    }

    void SetSkinningAnimationClip(
        std::string clip_name,
        std::optional<std::uint32_t> clip_index = std::nullopt)
    {
        skinning_animation_clip_name_ = std::move(clip_name);
        skinning_animation_clip_index_ = clip_index;
    }

    bool IsSkinningAnimationEnabled() const
    {
        return skinning_animation_enabled_;
    }

    float GetSkinningAnimationSpeed() const
    {
        return skinning_animation_speed_;
    }

    const std::string& GetSkinningAnimationClipName() const
    {
        return skinning_animation_clip_name_;
    }

    std::optional<std::uint32_t> GetSkinningAnimationClipIndex() const
    {
        return skinning_animation_clip_index_;
    }

    void SetBoneMatricesCallback(
        std::function<std::vector<glm::mat4>(double)> callback)
    {
        bone_matrices_callback_ = std::move(callback);
    }

    bool HasBoneMatricesCallback() const
    {
        return static_cast<bool>(bone_matrices_callback_);
    }

    std::vector<glm::mat4> EvaluateBoneMatrices(double time_s) const
    {
        if (!bone_matrices_callback_)
        {
            return {};
        }
        return bone_matrices_callback_(time_s);
    }

    double GetSkinningTime(double time_s) const
    {
        if (!skinning_animation_enabled_)
        {
            return 0.0;
        }
        return time_s * static_cast<double>(skinning_animation_speed_);
    }

    void SetRaytraceTriangleCallback(
        std::function<std::vector<float>(double)> callback)
    {
        raytrace_triangle_callback_ = std::move(callback);
    }

    bool HasRaytraceTriangleCallback() const
    {
        return static_cast<bool>(raytrace_triangle_callback_);
    }

    bool HasActiveRaytraceTriangleCallback() const
    {
        return HasRaytraceTriangleCallback() && skinning_animation_enabled_;
    }

    std::vector<float> EvaluateRaytraceTriangles(double time_s) const
    {
        if (!raytrace_triangle_callback_)
        {
            return {};
        }
        return raytrace_triangle_callback_(time_s);
    }

    void SetRaytraceBvhCallback(
        std::function<std::vector<BVHNode>(double)> callback)
    {
        raytrace_bvh_callback_ = std::move(callback);
    }

    bool HasRaytraceBvhCallback() const
    {
        return static_cast<bool>(raytrace_bvh_callback_);
    }

    bool HasActiveRaytraceBvhCallback() const
    {
        return HasRaytraceBvhCallback() && skinning_animation_enabled_;
    }

    std::vector<BVHNode> EvaluateRaytraceBvh(double time_s) const
    {
        if (!raytrace_bvh_callback_)
        {
            return {};
        }
        return raytrace_bvh_callback_(time_s);
    }

    void SetGpuSkinningSourceData(
        std::vector<float> points,
        std::vector<float> normals,
        std::vector<float> textures,
        std::vector<std::uint32_t> triangle_indices,
        std::vector<std::int32_t> bone_indices,
        std::vector<float> bone_weights)
    {
        skinning_points_ = std::move(points);
        skinning_normals_ = std::move(normals);
        skinning_textures_ = std::move(textures);
        skinning_triangle_indices_ = std::move(triangle_indices);
        skinning_bone_indices_ = std::move(bone_indices);
        skinning_bone_weights_ = std::move(bone_weights);
    }

    bool HasGpuSkinningSourceData() const
    {
        const std::size_t vertex_count = skinning_points_.size() / 3u;
        return vertex_count > 0 &&
               !skinning_triangle_indices_.empty() &&
               skinning_bone_indices_.size() >= vertex_count * 4u &&
               skinning_bone_weights_.size() >= vertex_count * 4u;
    }

    const std::vector<float>& GetSkinningPoints() const
    {
        return skinning_points_;
    }

    const std::vector<float>& GetSkinningNormals() const
    {
        return skinning_normals_;
    }

    const std::vector<float>& GetSkinningTextures() const
    {
        return skinning_textures_;
    }

    const std::vector<std::uint32_t>& GetSkinningTriangleIndices() const
    {
        return skinning_triangle_indices_;
    }

    const std::vector<std::int32_t>& GetSkinningBoneIndices() const
    {
        return skinning_bone_indices_;
    }

    const std::vector<float>& GetSkinningBoneWeights() const
    {
        return skinning_bone_weights_;
    }

  private:
    bool skinning_animation_enabled_ = false;
    float skinning_animation_speed_ = 1.0f;
    std::string skinning_animation_clip_name_ = {};
    std::optional<std::uint32_t> skinning_animation_clip_index_ = std::nullopt;
    std::function<std::vector<glm::mat4>(double)> bone_matrices_callback_ =
        nullptr;
    std::function<std::vector<float>(double)> raytrace_triangle_callback_ =
        nullptr;
    std::function<std::vector<BVHNode>(double)> raytrace_bvh_callback_ =
        nullptr;
    std::vector<float> skinning_points_ = {};
    std::vector<float> skinning_normals_ = {};
    std::vector<float> skinning_textures_ = {};
    std::vector<std::uint32_t> skinning_triangle_indices_ = {};
    std::vector<std::int32_t> skinning_bone_indices_ = {};
    std::vector<float> skinning_bone_weights_ = {};
};

} // namespace frame::vulkan
