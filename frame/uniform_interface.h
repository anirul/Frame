#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "frame/json/proto.h"
#include "frame/serialize.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace frame
{

/**
 * @class UniformInterface
 * @brief An interface to a uniform.
 *
 * This class is an abstractiion of a uniform. It is used to be returned by
 * the uniform collection as a reference to a sisngle uniform.
 */
class UniformInterface : public Serialize<proto::Uniform>
{
  public:
    //! @brief Virtual destructor.
    virtual ~UniformInterface() = default;
};

} // End namespace frame.
