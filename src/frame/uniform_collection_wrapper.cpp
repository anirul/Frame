#include "frame/uniform_collection_wrapper.h"
#include "frame/uniform.h"

#include "frame/node_matrix.h"

namespace frame
{

UniformCollectionWrapper::UniformCollectionWrapper(
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model,
    double time)
{
    std::unique_ptr<Uniform> uniform_projection =
        std::make_unique<Uniform>("projection", projection);
    std::unique_ptr<Uniform> uniform_view =
        std::make_unique<Uniform>("view", view);
    std::unique_ptr<Uniform> uniform_model =
        std::make_unique<Uniform>("model", model);
    std::unique_ptr<Uniform> uniform_time =
        std::make_unique<Uniform>("time", static_cast<float>(time));
    std::unique_ptr<Uniform> uniform_time_s =
        std::make_unique<Uniform>("time_s", static_cast<float>(time));
    AddUniform(std::move(uniform_projection));
    AddUniform(std::move(uniform_view));
    AddUniform(std::move(uniform_model));
    AddUniform(std::move(uniform_time));
    AddUniform(std::move(uniform_time_s));
}

const UniformInterface& UniformCollectionWrapper::GetUniform(
    const std::string& name) const
{
    auto it = value_map_.find(name);
    if (it != value_map_.end())
    {
        return *(it->second);
    }
    throw std::runtime_error("Uniform not found");
}

void UniformCollectionWrapper::AddUniform(
    std::unique_ptr<UniformInterface>&& uniform)
{
    auto it = value_map_.find(uniform->GetName());
    if (it != value_map_.end())
    {
        it->second = std::move(uniform);
    }
    else
    {
        value_map_[uniform->GetName()] = std::move(uniform);
    }
}

void UniformCollectionWrapper::RemoveUniform(const std::string& name)
{
    auto it = value_map_.find(name);
    if (it != value_map_.end())
    {
        value_map_.erase(it);
    }
}

std::vector<std::string> UniformCollectionWrapper::GetUniformNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : value_map_)
    {
        names.push_back(pair.first);
    }
    return names;
}

} // End namespace frame.
