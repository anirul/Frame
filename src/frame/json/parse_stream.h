#pragma once

#include <memory>

#include "frame/json/proto.h"
#include "frame/stream_interface.h"

namespace frame::proto {

/**
 * @brief Parse a texture stream from a proto description.
 * @param proto_texture: The input proto.
 * @return A unique pointer to a stream of texture interface.
 */
std::vector<StreamInterface<std::uint8_t>*> ParseAllTextureStream(
    const proto::Texture& proto_texture);
/**
 * @brief Parse a buffer stream from a proto description.
 * @param proto_buffer: The input proto as a scene static mesh.
 * @return A unique pointer to a stream of buffer interface.
 */
StreamInterface<float>* ParseNoneBufferStream(const proto::SceneStaticMesh& proto_buffer);
/**
 * @brief Parse a uniform stream from a proto description.
 * @param proto_uniform: The input proto as a uniform.
 * @return A unique pointer to a stream of uniform interface.
 */
StreamInterface<float>* ParseNoneUniformFloatStream(const proto::Uniform& proto_uniform);
/**
 * @brief Parse all the uniform from a stream.
 * @param proto_uniform: The input proto as a uniform.
 * @return A vector of unique pointer to uniform interfaces.
 */
std::vector<StreamInterface<float>*> ParseAllUniformFloatStream(
    const proto::Uniform& proto_uniform);
/**
 * @brief Parse a uniform stream from a proto description.
 * @param proto_uniform: The input proto as a uniform.
 * @return A unique pointer to a stream of uniform interface.
 */
StreamInterface<std::int32_t>* ParseNoneUniformIntStream(const proto::Uniform& proto_uniform);
/**
 * @brief Parse all the uniform from a stream.
 * @param proto_uniform: The input proto as a uniform.
 * @return A vector of unique pointer to uniform interfaces.
 */
std::vector<StreamInterface<std::int32_t>*> ParseAllUniformIntStream(
    const proto::Uniform& proto_uniform);

}  // End namespace frame::proto.
