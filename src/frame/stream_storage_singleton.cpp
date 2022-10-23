#include "frame/stream_storage_singleton.h"

#include <fmt/core.h>

#include <cassert>
#include <stdexcept>

namespace frame {

template <typename T>
StreamInterface<T>* StreamStorageSingleton::ConnectToSpecificStreamInterface(
    const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
    const std::string& connect, std::uint32_t stream_id) {
    // Create the stream name from stream id.
    std::string connect_string = fmt::format("camera[{}].{}", stream_id, connect);
    if (!map.count(connect_string)) {
        throw std::runtime_error(
            fmt::format("Could not find any element [{}] in the map.", connect_string));
    }
    return map.at(connect_string).get();
}

template <typename T>
StreamInterface<T>* StreamStorageSingleton::ConnectToNoneStreamInterface(
    const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
    const std::string& connect) {
    std::string connect_string = connect;
    if (!map.count(connect_string)) {
        throw std::runtime_error(
            fmt::format("Could not find any element [{}] in the map.", connect_string));
    }
    return map.at(connect_string).get();
}

template <typename T>
std::vector<StreamInterface<T>*> StreamStorageSingleton::ConnectToAllStreamInterface(
    const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
    const std::string& connect) {
    std::vector<StreamInterface<T>*> list;
    for (const auto& [_, number] : id_number_map_) {
        list.emplace_back(ConnectToSpecificStreamInterface(map, connect, number));
    }
    return list;
}

StreamStorageSingleton& StreamStorageSingleton::GetInstance() {
    static StreamStorageSingleton instance;
    return instance;
}

void StreamStorageSingleton::SetStreamBufferCallback(
    const std::string& connect, std::unique_ptr<StreamInterface<float>>&& callbacks) {
    logger_->info("Set stream from [{}] to [{}]", connect, callbacks->GetName());
    name_stream_map_.insert({ callbacks->GetName(), connect });
    buffer_callback_map_.insert({ connect, std::move(callbacks) });
}

void StreamStorageSingleton::SetStreamTextureCallback(
    const std::string& connect, std::unique_ptr<StreamInterface<std::uint8_t>>&& callbacks) {
    logger_->info("Set stream from [{}] to [{}]", connect, callbacks->GetName());
    name_stream_map_.insert({ callbacks->GetName(), connect });
    texture_callback_map_.insert({ connect, std::move(callbacks) });
}

void StreamStorageSingleton::SetStreamUniformFloatCallback(
    const std::string& connect, std::unique_ptr<StreamInterface<float>>&& callbacks) {
    logger_->info("Set stream from [{}] to [{}]", connect, callbacks->GetName());
    name_stream_map_.insert({ callbacks->GetName(), connect });
    uniform_float_callback_map_.insert({ connect, std::move(callbacks) });
}

void StreamStorageSingleton::SetStreamUniformIntCallback(
    const std::string& connect, std::unique_ptr<StreamInterface<std::int32_t>>&& callbacks) {
    logger_->info("Set stream from [{}] to [{}]", connect, callbacks->GetName());
    name_stream_map_.insert({ callbacks->GetName(), connect });
    uniform_int_callback_map_.insert({ connect, std::move(callbacks) });
}

void StreamStorageSingleton::SetRootId(const std::string& root_id) { default_id_ = root_id; }

std::string StreamStorageSingleton::GetRootId() const { return default_id_; }

std::uint32_t StreamStorageSingleton::AddId(const std::string& id) {
    if (!id_number_map_.count(id)) {
        id_number_map_.insert({ id, static_cast<std::uint32_t>(id_number_map_.size()) });
    }
    return GetIdNumber(id);
}

std::uint32_t StreamStorageSingleton::GetIdNumber(const std::string& id) const {
    return id_number_map_.at(id);
}

StreamInterface<float>* StreamStorageSingleton::ConnectToNoneStreamBufferInterface(
    const std::string& connect) {
    return ConnectToNoneStreamInterface<float>(buffer_callback_map_, connect);
}

std::vector<StreamInterface<std::uint8_t>*>
StreamStorageSingleton::ConnectToAllStreamTextureInterface(const std::string& connect) {
    return ConnectToAllStreamInterface<std::uint8_t>(texture_callback_map_, connect);
}

StreamInterface<float>* StreamStorageSingleton::ConnectToNoneStreamUniformFloatInterface(
    const std::string& connect) {
    return ConnectToNoneStreamInterface<float>(uniform_float_callback_map_, connect);
}

std::vector<StreamInterface<float>*>
StreamStorageSingleton::ConnectToAllStreamUniformFloatInterface(const std::string& connect) {
    return ConnectToAllStreamInterface<float>(uniform_float_callback_map_, connect);
}

StreamInterface<std::int32_t>* StreamStorageSingleton::ConnectToNoneStreamUniformIntInterface(
    const std::string& connect) {
    return ConnectToNoneStreamInterface<std::int32_t>(uniform_int_callback_map_, connect);
}

std::vector<StreamInterface<std::int32_t>*>
StreamStorageSingleton::ConnectToAllStreamUniformIntInterface(const std::string& connect) {
    return ConnectToAllStreamInterface<std::int32_t>(uniform_int_callback_map_, connect);
}

std::string StreamStorageSingleton::GetItemNameFromStreamName(const std::string& name) const {
    return name_stream_map_.at(name);
}

bool StreamStorageSingleton::HasStreamName(const std::string& name) const {
    return static_cast<bool>(name_stream_map_.count(name));
}

}  // End namespace frame.
