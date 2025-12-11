#include "frame/vulkan/scene_state.h"

#include <glm/gtc/matrix_inverse.hpp>

#include "frame/camera.h"

namespace frame::vulkan
{

SceneState BuildSceneState(
    frame::LevelInterface& level,
    frame::Logger& logger,
    glm::uvec2 swapchain_extent,
    float elapsed_time_seconds,
    frame::EntityId preferred_material)
{
    SceneState state;

    try
    {
        frame::Camera camera_for_frame(level.GetDefaultCamera());
        auto camera_holder_id = level.GetDefaultCameraId();
        if (camera_holder_id != frame::NullId)
        {
            auto& node = level.GetSceneNodeFromId(camera_holder_id);
            auto matrix_node =
                node.GetLocalModel(static_cast<double>(elapsed_time_seconds));
            auto inverse_model = glm::inverse(matrix_node);
            camera_for_frame.SetFront(
                level.GetDefaultCamera().GetFront() * glm::mat3(inverse_model));
            camera_for_frame.SetPosition(glm::vec3(
                glm::vec4(level.GetDefaultCamera().GetPosition(), 1.0f) *
                inverse_model));
        }

        if (swapchain_extent.y != 0)
        {
            camera_for_frame.SetAspectRatio(
                static_cast<float>(swapchain_extent.x) /
                static_cast<float>(swapchain_extent.y));
        }
        state.projection = camera_for_frame.ComputeProjection();
        state.projection[1][1] *= -1.0f;
        state.view = camera_for_frame.ComputeView();
        glm::mat4 rotation = glm::mat4(1.0f);
        state.view = rotation * state.view;
        state.camera_position = camera_for_frame.GetPosition();
    }
    catch (const std::exception& ex)
    {
        logger->warn("Failed to compute camera state: {}", ex.what());
    }

    try
    {
        bool model_set = false;
        if (preferred_material != frame::NullId)
        {
            auto& material = level.GetMaterialFromId(preferred_material);
            for (const auto& node_name : material.GetNodeNames())
            {
                auto node_id = level.GetIdFromName(node_name);
                if (node_id != frame::NullId)
                {
                    auto& node = level.GetSceneNodeFromId(node_id);
                    state.model = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds));
                    model_set = true;
                    break;
                }
            }
            // Special-case legacy AppleMesh naming: if present, resolve directly.
            if (!model_set)
            {
                auto apple_id = level.GetIdFromName("AppleMesh");
                if (apple_id != frame::NullId)
                {
                    auto& node = level.GetSceneNodeFromId(apple_id);
                    state.model = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds));
                    model_set = true;
                }
            }
        }
        if (!model_set && preferred_material != frame::NullId)
        {
            for (const auto& pair : level.GetStaticMeshMaterialIds())
            {
                if (pair.second == preferred_material)
                {
                    auto& node = level.GetSceneNodeFromId(pair.first);
                    state.model = node.GetLocalModel(
                        static_cast<double>(elapsed_time_seconds));
                    model_set = true;
                    break;
                }
            }
        }
        if (!model_set && preferred_material != frame::NullId)
        {
            // Search all render-time buckets for a mesh using this material.
            for (auto render_time :
                 {frame::proto::NodeStaticMesh::SCENE_RENDER_TIME,
                  frame::proto::NodeStaticMesh::PRE_RENDER_TIME,
                  frame::proto::NodeStaticMesh::POST_PROCESS_TIME,
                  frame::proto::NodeStaticMesh::SKYBOX_RENDER_TIME,
                  frame::proto::NodeStaticMesh::SHADOW_RENDER_TIME})
            {
                const auto pairs = level.GetStaticMeshMaterialIds(render_time);
                for (const auto& pair : pairs)
                {
                    if (pair.second == preferred_material)
                    {
                        auto& node = level.GetSceneNodeFromId(pair.first);
                        state.model = node.GetLocalModel(
                            static_cast<double>(elapsed_time_seconds));
                        model_set = true;
                        break;
                    }
                }
                if (model_set)
                {
                    break;
                }
            }
        }
        if (!model_set)
        {
            const auto mesh_pairs = level.GetStaticMeshMaterialIds();
            if (!mesh_pairs.empty())
            {
                auto node_id = mesh_pairs.front().first;
                auto& node = level.GetSceneNodeFromId(node_id);
                state.model = node.GetLocalModel(
                    static_cast<double>(elapsed_time_seconds));
            }
        }
    }
    catch (const std::exception& ex)
    {
        logger->warn("Failed to compute model matrix: {}", ex.what());
    }

    try
    {
        const auto lights = level.GetLights();
        if (!lights.empty())
        {
            auto& light = level.GetLightFromId(lights.front());
            state.light_dir = light.GetVector();
            state.light_color = light.GetColorIntensity();
        }
    }
    catch (const std::exception& ex)
    {
        logger->warn("Failed to fetch light information: {}", ex.what());
    }

    return state;
}

UniformBlock MakeUniformBlock(
    const SceneState& state, float elapsed_time_seconds)
{
    UniformBlock block{};
    block.projection = state.projection;
    block.view = state.view;
    block.projection_inv = glm::inverse(state.projection);
    block.view_inv = glm::inverse(state.view);
    block.model = state.model;
    block.model_inv = glm::inverse(state.model);
    block.camera_position = glm::vec4(state.camera_position, 1.0f);
    block.light_dir = glm::vec4(state.light_dir, 0.0f);
    block.light_color = glm::vec4(state.light_color, 1.0f);
    block.time_s = glm::vec4(elapsed_time_seconds, 0.0f, 0.0f, 0.0f);
    return block;
}

} // namespace frame::vulkan
