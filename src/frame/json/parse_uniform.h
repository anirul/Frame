#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "frame/json/proto.h"
#include "frame/program_interface.h"
#include "frame/uniform.h"
#include "frame/uniform_collection_interface.h"

namespace frame::json
{

/**
 * @file From proto to glm.
 */

/**
 * @brief First start with simple type, this is the template part (int, float,
 *        short,...).
 * @param uniform_val: Value to be transfered to a C++ value.
 */
template <typename T> T ParseUniform(const T& uniform_val)
{
    return uniform_val;
}
/**
 * @brief Specialization into glm types (vec2).
 * @param uniform_vec2: Uniform vector 2 input.
 * @return Glm vec2 output.
 */
glm::vec2 ParseUniform(const proto::UniformVector2& uniform_vec2);
/**
 * @brief Specialization into glm types (vec3).
 * @param uniform_vec3: Uniform vector 3 input.
 * @return Glm vec3 output.
 */
glm::vec3 ParseUniform(const proto::UniformVector3& uniform_vec3);
/**
 * @brief Specialization into glm types (vec4).
 * @param uniform_vec4: Uniform vector 4 input.
 * @return Glm vec4 output.
 */
glm::vec4 ParseUniform(const proto::UniformVector4& uniform_vec4);
/**
 * @brief Specialization into glm types (mat4).
 * @param uniform_mat4: Uniform matrix input.
 * @return Glm mat4 output.
 */
glm::mat4 ParseUniform(const proto::UniformMatrix4& uniform_mat4);
/**
 * @brief Specialization into vector types.
 * @param name: Name of the uniform.
 * @param uniform_vec: The vector value.
 * @param program_interface: Program interface to set the uniform.
 */
template <typename T, typename U>
void ParseUniformVec(
    const std::string& name,
    const U& uniform_vec,
    ProgramInterface& program_interface)
{
    std::uint32_t counter = 0;
    for (const T& uniform_val : uniform_vec.values())
    {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name + '[' + std::to_string(counter++) + ']',
                ParseUniform(uniform_val));
        program_interface.AddUniform(std::move(uniform_interface));
    }
}
/**
 * @brief Register the uniform in the program.
 * @param uniform: Proto uniform entry.
 * @param uniform_collection_interface: Reference to a uniform interface.
 * @param program_interface: Reference to a program interface.
 */
void RegisterUniformFromProto(
    const frame::proto::Uniform& uniform,
    const UniformCollectionInterface& uniform_collection_interface,
    ProgramInterface& program_interface);
/**
 * @brief Register the uniform enum in the program.
 * @param name: Uniform name.
 * @param uniform_enum: Uniform enum.
 * @param uniform_interface: Uniform interface.
 * @param program_interface: Program interface.
 */
void RegisterUniformEnumFromProto(
    const std::string& name,
    const frame::proto::Uniform::UniformEnum& uniform_enum,
    const UniformCollectionInterface& uniform_interface,
    ProgramInterface& program_interface);
/**
 * @brief Convert frame proto size into pair.
 * @param size: Proto size param.
 * @return Size result.
 */
std::pair<std::uint32_t, std::uint32_t> ParseSizeInt(
    const frame::proto::Size size);

} // namespace frame::json.
