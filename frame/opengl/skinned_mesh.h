#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "frame/bvh.h"
#include "frame/opengl/mesh.h"

namespace frame::opengl
{

/**
 * @class SkinnedMesh
 * @brief OpenGL mesh with skeleton animation support.
 */
class SkinnedMesh : public Mesh
{
  public:
    SkinnedMesh(LevelInterface& level, const MeshParameter& parameters);
    ~SkinnedMesh() override;

  public:
    void SetSkinningBuffers(
        EntityId bone_index_buffer_id,
        EntityId bone_weight_buffer_id,
        std::uint32_t bone_index_buffer_size = 4,
        std::uint32_t bone_weight_buffer_size = 4);
    void SetSkinningCallback(
        std::function<std::vector<glm::mat4>(double)> callback);
    void SetSkinningAnimation(bool enabled, float speed = 1.0f);
    void SetSkinningAnimationClip(
        std::string clip_name,
        std::optional<std::uint32_t> clip_index = std::nullopt);
    void SetRaytraceTriangleCallback(
        std::function<std::vector<float>(double)> callback);
    void SetRaytraceBvhCallback(
        std::function<std::vector<BVHNode>(double)> callback);

    bool HasSkinning() const;
    bool IsSkinningAnimationEnabled() const;
    float GetSkinningAnimationSpeed() const;
    const std::string& GetSkinningAnimationClipName() const;
    std::optional<std::uint32_t> GetSkinningAnimationClipIndex() const;
    double GetSkinningTime(double time_s) const;
    std::vector<glm::mat4> EvaluateSkinning(double time_s) const;
    bool HasRaytraceTriangleCallback() const;
    std::vector<float> EvaluateRaytraceTriangles(double time_s) const;
    bool HasRaytraceBvhCallback() const;
    std::vector<BVHNode> EvaluateRaytraceBvh(double time_s) const;

  private:
    EntityId bone_index_buffer_id_ = NullId;
    std::uint32_t bone_index_buffer_size_ = 4;
    EntityId bone_weight_buffer_id_ = NullId;
    std::uint32_t bone_weight_buffer_size_ = 4;
    std::function<std::vector<glm::mat4>(double)> skinning_callback_ = nullptr;
    std::function<std::vector<float>(double)> raytrace_triangle_callback_ =
        nullptr;
    std::function<std::vector<BVHNode>(double)> raytrace_bvh_callback_ =
        nullptr;
    bool skinning_animation_enabled_ = false;
    float skinning_animation_speed_ = 1.0f;
    std::string skinning_animation_clip_name_ = {};
    std::optional<std::uint32_t> skinning_animation_clip_index_ = std::nullopt;
};

} // End namespace frame::opengl.

