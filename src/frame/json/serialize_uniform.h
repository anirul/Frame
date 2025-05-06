#pragma once

#include "frame/json/proto.h"
#include "frame/uniform_interface.h"

namespace frame::proto
{

/**
 * @brief Serialize uniform to their constituant proto.
 * @param uniform_interface: The interface to a uniform.
 * @return Proto that represent a uniform.
 */
proto::Uniform SerializeUniform(
    const frame::UniformInterface& uniform_interface);

} // namespace frame::proto