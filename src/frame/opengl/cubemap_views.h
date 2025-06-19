#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace frame::opengl
{

/**
 * @brief View matrices for cubemap rendering.
 */
// +X face
inline const std::array<glm::mat4, 6> kViewsCubemap = {
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)),
    // -X face
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)),
    // +Y face
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)),
    // -Y face
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)),
    // +Z face
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)),
    // -Z face
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f))};

/**
 * @brief Projection matrix for cubemap rendering.
 */
inline const glm::mat4 kProjectionCubemap =
    glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f);

} // End namespace frame::opengl
