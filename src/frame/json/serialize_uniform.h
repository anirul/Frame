#pragma once

#include "frame/json/proto.h"
#include "frame/uniform_interface.h"

namespace frame::proto
{

/**
 * @brief Serialize a glm::vec2 into a proto::UniformVector2.
 * @param vec : Vector to be serialized.
 * @return Proto serialized.
 */
proto::UniformVector2 SerializeUniformVector2(glm::vec2 vec);
/**
 * @brief Serialize a glm::vec3 into a proto::UniformVector3.
 * @param vec : Vector to be serialized.
 * @return Proto serialized.
 */
proto::UniformVector3 SerializeUniformVector3(glm::vec3 vec);
/**
 * @brief Serialize a glm::vec4 into a proto::UniformVector4.
 * @param vec : Vector to be serialized.
 * @return Proto serialized.
 */
proto::UniformVector4 SerializeUniformVector4(glm::vec4 vec);
/**
 * @brief Serialize a glm::mat4 into a proto::UniformMatrix4.
 * @param mat : Matrix to be serialized.
 * @return Proto serialized.
 */
proto::UniformMatrix4 SerializeUniformMatrix4(glm::mat4 mat);
/**
 * @brief Serialize uniform to their constituant proto.
 * @param uniform_interface: The interface to a uniform.
 * @return Proto that represent a uniform.
 */
proto::Uniform SerializeUniform(
    const frame::UniformInterface& uniform_interface);

} // namespace frame::proto