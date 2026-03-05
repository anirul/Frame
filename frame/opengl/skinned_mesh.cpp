#include "frame/opengl/skinned_mesh.h"

#include <cmath>
#include <glad/glad.h>
#include <utility>

#include "frame/opengl/buffer.h"

namespace frame::opengl
{

SkinnedMesh::SkinnedMesh(
    LevelInterface& level, const MeshParameter& parameters)
    : Mesh(level, parameters)
{
}

SkinnedMesh::~SkinnedMesh()
{
    if (bone_index_buffer_id_)
    {
        level_.RemoveBuffer(bone_index_buffer_id_);
    }
    if (bone_weight_buffer_id_)
    {
        level_.RemoveBuffer(bone_weight_buffer_id_);
    }
}

void SkinnedMesh::SetSkinningBuffers(
    EntityId bone_index_buffer_id,
    EntityId bone_weight_buffer_id,
    std::uint32_t bone_index_buffer_size,
    std::uint32_t bone_weight_buffer_size)
{
    bone_index_buffer_id_ = bone_index_buffer_id;
    bone_weight_buffer_id_ = bone_weight_buffer_id;
    bone_index_buffer_size_ = bone_index_buffer_size;
    bone_weight_buffer_size_ = bone_weight_buffer_size;

    glBindVertexArray(vertex_array_object_);

    auto& bone_index_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(bone_index_buffer_id_));
    bone_index_buffer.Bind();
    glVertexAttribIPointer(
        3,
        static_cast<GLint>(bone_index_buffer_size_),
        GL_INT,
        0,
        nullptr);
    bone_index_buffer.UnBind();
    glEnableVertexAttribArray(3);

    auto& bone_weight_buffer =
        dynamic_cast<Buffer&>(level_.GetBufferFromId(bone_weight_buffer_id_));
    bone_weight_buffer.Bind();
    glVertexAttribPointer(
        4,
        static_cast<GLint>(bone_weight_buffer_size_),
        GL_FLOAT,
        GL_FALSE,
        0,
        nullptr);
    bone_weight_buffer.UnBind();
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

void SkinnedMesh::SetSkinningCallback(
    std::function<std::vector<glm::mat4>(double)> callback)
{
    skinning_callback_ = std::move(callback);
}

void SkinnedMesh::SetSkinningAnimation(bool enabled, float speed)
{
    skinning_animation_enabled_ = enabled;
    if (!std::isfinite(speed))
    {
        speed = 1.0f;
    }
    skinning_animation_speed_ = speed;
}

void SkinnedMesh::SetSkinningAnimationClip(
    std::string clip_name,
    std::optional<std::uint32_t> clip_index)
{
    skinning_animation_clip_name_ = std::move(clip_name);
    skinning_animation_clip_index_ = clip_index;
}

void SkinnedMesh::SetRaytraceTriangleCallback(
    std::function<std::vector<float>(double)> callback)
{
    raytrace_triangle_callback_ = std::move(callback);
}

void SkinnedMesh::SetRaytraceBvhCallback(
    std::function<std::vector<BVHNode>(double)> callback)
{
    raytrace_bvh_callback_ = std::move(callback);
}

bool SkinnedMesh::HasSkinning() const
{
    return static_cast<bool>(skinning_callback_);
}

bool SkinnedMesh::IsSkinningAnimationEnabled() const
{
    return skinning_animation_enabled_;
}

float SkinnedMesh::GetSkinningAnimationSpeed() const
{
    return skinning_animation_speed_;
}

const std::string& SkinnedMesh::GetSkinningAnimationClipName() const
{
    return skinning_animation_clip_name_;
}

std::optional<std::uint32_t> SkinnedMesh::GetSkinningAnimationClipIndex() const
{
    return skinning_animation_clip_index_;
}

double SkinnedMesh::GetSkinningTime(double time_s) const
{
    if (!HasSkinning() || !skinning_animation_enabled_)
    {
        return 0.0;
    }
    return time_s * static_cast<double>(skinning_animation_speed_);
}

std::vector<glm::mat4> SkinnedMesh::EvaluateSkinning(double time_s) const
{
    if (!skinning_callback_)
    {
        return {};
    }
    return skinning_callback_(time_s);
}

bool SkinnedMesh::HasRaytraceTriangleCallback() const
{
    return static_cast<bool>(raytrace_triangle_callback_);
}

std::vector<float> SkinnedMesh::EvaluateRaytraceTriangles(double time_s) const
{
    if (!raytrace_triangle_callback_)
    {
        return {};
    }
    return raytrace_triangle_callback_(time_s);
}

bool SkinnedMesh::HasRaytraceBvhCallback() const
{
    return static_cast<bool>(raytrace_bvh_callback_);
}

std::vector<BVHNode> SkinnedMesh::EvaluateRaytraceBvh(double time_s) const
{
    if (!raytrace_bvh_callback_)
    {
        return {};
    }
    return raytrace_bvh_callback_(time_s);
}

} // End namespace frame::opengl.

