#include "light.h"

#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace frame::opengl
{

glm::mat4 LightPoint::ComputeView(const CameraInterface& camera) const
{
    // 1. Use the point light’s world position (assuming GetVector() is
    // position).
    glm::vec3 lightPos = GetVector();

    // 2. Use the camera’s orientation
    glm::vec3 front = glm::normalize(camera.GetFront());
    glm::vec3 up = glm::normalize(camera.GetUp());

    // 3. Build a single view matrix
    //    We’ll “look” in the camera’s front direction from the light’s
    //    position. This only makes sense if you want a single direction of
    //    shadow from the point light. (For a true cubemap, you do 6 passes or
    //    something else.)
    return glm::lookAt(
        lightPos,         // eye
        lightPos + front, // target
        up);
}

glm::mat4 LightDirectional::ComputeView(const CameraInterface& camera) const
{
    // 1. The light’s direction from your proto (already stored in
    // LightDirectional).
    //    e.g. "GetVector()" returns the directional vector.
    glm::vec3 dir = glm::normalize(GetVector());

    // 2. Use the camera’s up vector so shadows remain consistent with how the
    // camera is oriented.
    glm::vec3 up = glm::normalize(camera.GetUp());

    // 3. Pick a “center” from the camera’s position (very naive):
    //    In many engines, we might actually compute a bounding box around what
    //    the camera sees, then figure out that box’s center.
    glm::vec3 center = camera.GetPosition();

    // 4. Position the light’s “eye” behind ‘center’ along ‘dir’.
    //    The distance is somewhat arbitrary or based on your scene size.
    float distance = 100.0f; // adjust as needed
    glm::vec3 eye = center - dir * distance;

    // 5. Build the view matrix
    return glm::lookAt(
        eye,    // “eye” or camera position
        center, // “center,” the point the light camera looks at
        up      // up vector from the real camera
    );
}

void LightManager::RegisterToProgram(Program& program) const
{
    if (lights_.size() > 32)
    {
        throw std::runtime_error("too many lights!");
    }
    program.Use();
    int i = 0;
    for (const auto& light : lights_)
    {
        program.Uniform(
            "light_position[" + std::to_string(i) + "]",
            lights_[i]->GetVector());
        program.Uniform(
            "light_color[" + std::to_string(i) + "]",
            lights_[i]->GetColorIntensity());
        ++i;
    }
    program.Uniform("light_max", static_cast<int>(lights_.size()));
    program.UnUse();
}

} // End namespace frame::opengl.
