#include "frame/uniform_wrapper.h"

#include "frame/node_matrix.h"

namespace frame {

UniformWrapper::UniformWrapper(const glm::mat4& projection,
                               const glm::mat4& view, const glm::mat4& model,
                               double time)
    : projection_(projection), view_(view), model_(model), time_(time) {}

glm::mat4 UniformWrapper::GetProjection() const { return projection_; }

glm::mat4 UniformWrapper::GetView() const { return view_; }

glm::mat4 UniformWrapper::GetModel() const { return model_; }

void UniformWrapper::SetValueFloat(const std::string& name,
                                   const std::vector<float>& vector,
                                   glm::uvec2 size) {
  if (stream_value_float_map_.count(name)) {
    stream_value_float_map_.erase(name);
  }
  stream_value_float_map_.insert({name, {vector, size}});
}

void UniformWrapper::SetValueInt(const std::string& name,
                                 const std::vector<std::int32_t>& vector,
                                 glm::uvec2 size) {
  if (stream_value_int_map_.count(name)) {
    stream_value_int_map_.erase(name);
  }
  stream_value_int_map_.insert({name, {vector, size}});
}

std::vector<float> UniformWrapper::GetValueFloat(
    const std::string& name) const {
  return stream_value_float_map_.at(name).value;
}

std::vector<std::int32_t> UniformWrapper::GetValueInt(
    const std::string& name) const {
  return stream_value_int_map_.at(name).value;
}

std::vector<std::string> UniformWrapper::GetFloatNames() const {
  std::vector<std::string> list;
  for (const auto& [name, _] : stream_value_float_map_) {
    list.push_back(name);
  }
  return list;
}

std::vector<std::string> UniformWrapper::GetIntNames() const {
  std::vector<std::string> list;
  for (const auto& [name, _] : stream_value_int_map_) {
    list.push_back(name);
  }
  return list;
}

glm::uvec2 UniformWrapper::GetSizeFromFloat(const std::string& name) const {
  return stream_value_float_map_.at(name).size;
}

glm::uvec2 UniformWrapper::GetSizeFromInt(const std::string& name) const {
  return stream_value_int_map_.at(name).size;
}

double UniformWrapper::GetDeltaTime() const { return time_; }

}  // End namespace frame.
