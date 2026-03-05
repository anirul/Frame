#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

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

    std::vector<BVHNode> EvaluateRaytraceBvh(double time_s) const
    {
        if (!raytrace_bvh_callback_)
        {
            return {};
        }
        return raytrace_bvh_callback_(time_s);
    }

  private:
    bool skinning_animation_enabled_ = false;
    float skinning_animation_speed_ = 1.0f;
    std::string skinning_animation_clip_name_ = {};
    std::optional<std::uint32_t> skinning_animation_clip_index_ = std::nullopt;
    std::function<std::vector<float>(double)> raytrace_triangle_callback_ =
        nullptr;
    std::function<std::vector<BVHNode>(double)> raytrace_bvh_callback_ =
        nullptr;
};

} // namespace frame::vulkan
