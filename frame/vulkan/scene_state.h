#pragma once

#include "frame/backend_internal.h"

#include <glm/glm.hpp>
#include <string>

#include "frame/level_interface.h"
#include "frame/logger.h"

namespace frame::vulkan
{

struct alignas(16) UniformBlock
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 projection_inv;
    glm::mat4 view_inv;
    glm::mat4 model;
    glm::mat4 model_inv;
    glm::mat4 env_map_model;
    glm::vec4 camera_position;
    glm::vec4 light_dir;
    glm::vec4 light_color;
    glm::vec4 time_s;
    glm::mat4 light_view_projection;
    glm::vec4 shadow_params;
};

struct SceneState
{
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 env_map_model = glm::mat4(1.0f);
    glm::vec3 camera_position = glm::vec3(0.0f);
    glm::vec3 light_dir = glm::vec3(0.0f);
    glm::vec3 light_color = glm::vec3(1.0f);
    float light_type = 0.0f;
    glm::mat4 light_view_projection = glm::mat4(1.0f);
    float shadow_enabled = 0.0f;
    float shadow_bias = 0.0035f;
    float shadow_map_size = 2048.0f;
};

SceneState BuildSceneState(
    frame::LevelInterface& level,
    frame::Logger& logger,
    glm::uvec2 swapchain_extent,
    float elapsed_time_seconds,
    frame::EntityId preferred_material = frame::NullId,
    bool flip_projection_y = true,
    const std::string& preferred_scene_root = {},
    bool force_identity_model = false);

UniformBlock MakeUniformBlock(
    const SceneState& state, float elapsed_time_seconds);

} // namespace frame::vulkan
