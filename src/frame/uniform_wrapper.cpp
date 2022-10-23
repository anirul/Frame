#include "frame/uniform_wrapper.h"

#include "frame/node_matrix.h"
#include "frame/stream_storage_singleton.h"

namespace frame {

glm::vec3 UniformWrapper::GetCameraPosition() const {
    if (camera_) return camera_->GetPosition();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraFront() const {
    if (camera_) return camera_->GetFront();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraRight() const {
    if (camera_) return camera_->GetRight();
    return glm::vec3(0.0f);
}

glm::vec3 UniformWrapper::GetCameraUp() const {
    if (camera_) return camera_->GetUp();
    return glm::vec3(0.0f);
}

glm::mat4 UniformWrapper::GetProjection() const {
    if (camera_) return camera_->ComputeProjection();
    return projection_;
}

glm::mat4 UniformWrapper::GetView() const {
    if (camera_) return camera_->ComputeView();
    return view_;
}

glm::mat4 UniformWrapper::GetModel() const { return model_; }

void UniformWrapper::SetValueFloatFromStream(const std::string& name, std::vector<float>& vector,
                                             std::pair<std::uint32_t, std::uint32_t> size) {
    std::string stream_name = name;
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
    }
    if (stream_value_float_map_.count(stream_name)) {
        stream_value_float_map_.erase(stream_name);
    }
    stream_value_float_map_.insert({ stream_name, { vector, size } });
}

void UniformWrapper::SetValueIntFromStream(const std::string& name,
                                           std::vector<std::int32_t>& vector,
                                           std::pair<std::uint32_t, std::uint32_t> size) {
    std::string stream_name = name;
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
    }
    if (stream_value_int_map_.count(stream_name)) {
        stream_value_int_map_.erase(stream_name);
    }
    stream_value_int_map_.insert({ stream_name, { vector, size } });
}

std::vector<float> UniformWrapper::GetValueFloatFromStream(const std::string& name) const {
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        auto stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
        return stream_value_float_map_.at(stream_name).value;
    }
    return stream_value_float_map_.at(name).value;
}

std::vector<std::int32_t> UniformWrapper::GetValueIntFromStream(const std::string& name) const {
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        auto stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
        return stream_value_int_map_.at(stream_name).value;
    }
    return stream_value_int_map_.at(name).value;
}

std::pair<std::uint32_t, std::uint32_t> UniformWrapper::GetSizeFromFloatStream(
    const std::string& name) const {
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        auto stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
        return stream_value_float_map_.at(stream_name).size;
    }
    return stream_value_float_map_.at(name).size;
}

std::pair<std::uint32_t, std::uint32_t> UniformWrapper::GetSizeFromIntStream(
    const std::string& name) const {
    if (StreamStorageSingleton::GetInstance().HasStreamName(name)) {
        auto stream_name = StreamStorageSingleton::GetInstance().GetItemNameFromStreamName(name);
        return stream_value_int_map_.at(stream_name).size;
    }
    return stream_value_int_map_.at(name).size;
}

double UniformWrapper::GetDeltaTime() const { return time_; }

}  // End namespace frame.
