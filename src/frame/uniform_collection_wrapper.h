#pragma once

#include "frame/level_interface.h"
#include "frame/uniform_collection_interface.h"

namespace frame
{

/**
 * @class UniformWrapper
 * @brief Get access to essential part of the rendering uniform system.
 *
 * This class is to be passed to the rendering system to be able to get the
 * enum uniform.
 */
class UniformCollectionWrapper : public UniformCollectionInterface
{
  public:
    /**
     * @brief Default constructor.
     */
    UniformCollectionWrapper() = default;
    /**
     * @brief Constructor create a wrapper from a camera pointer.
     * @param Camera pointer.
     */
    UniformCollectionWrapper(
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::mat4& model,
        double dt);

  public:
    /**
     * @brief Get the uniform.
     * @return The uniform.
     */
    const UniformInterface& GetUniform(const std::string& name) const override;
    /**
     * @brief Set the uniform.
     * @param uniform: The uniform to set.
     */
    void AddUniform(std::unique_ptr<UniformInterface>&& uniform) override;
    /**
     * @brief Remove a uniform from the collection.
     * @param name: The name of the uniform to be removed.
     */
    void RemoveUniform(const std::string& name) override;
    /**
     * @brief Get the uniform names.
     * @return The uniform names.
     */
    const std::vector<std::string> GetUniformNames() const override;

  private:
    std::map<std::string, std::unique_ptr<UniformInterface>> value_map_ = {};
};

} // End namespace frame.
