#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include "uniform_interface.h"

namespace frame
{

/**
 * @class UniformCollectionInterface
 * @brief Get access to essential part of the rendering uniform system.
 *
 * This class (and a derived version of it) is to be passed to the rendering
 * system to be able to get the enum uniform.
 */
class UniformCollectionInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~UniformCollectionInterface() = default;
    /**
     * @brief Get the uniform.
     * @return The uniform.
     */
    virtual const UniformInterface& GetUniform(
		const std::string& name) const = 0;
    /**
     * @brief Set the uniform.
     * @param uniform: The uniform to set.
     */
    virtual void AddUniform(std::unique_ptr<UniformInterface>&& uniform) = 0;
    /**
     * @brief Remove a uniform from the collection.
	 * @param name: The name of the uniform to be removed.
	 */
    virtual void RemoveUniform(const std::string& name) = 0;
    /**
     * @brief Get the uniform names.
     * @return The uniform names.
     */
    virtual std::vector<std::string> GetUniformNames() const = 0;
};

} // End namespace frame.
