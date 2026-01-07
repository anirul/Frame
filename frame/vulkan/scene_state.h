#pragma once

#include <glm/glm.hpp>

#include "frame/logger.h"
#include "frame/level_interface.h"

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
};

SceneState BuildSceneState(
    frame::LevelInterface& level,
    frame::Logger& logger,
    glm::uvec2 swapchain_extent,
    float elapsed_time_seconds,
    frame::EntityId preferred_material = frame::NullId,
    bool flip_projection_y = true);

UniformBlock MakeUniformBlock(
    const SceneState& state, float elapsed_time_seconds);

} // namespace frame::vulkan
