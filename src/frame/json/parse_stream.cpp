#include "frame/json/parse_stream.h"

#include "frame/stream_interface.h"
#include "frame/stream_storage_singleton.h"

namespace frame::proto {

std::vector<StreamInterface<std::uint8_t>*> ParseAllTextureStream(
    const proto::Texture& proto_texture) {
    return StreamStorageSingleton::GetInstance().ConnectToAllStreamTextureInterface(
        proto_texture.name());
}

StreamInterface<float>* ParseNoneBufferStream(const proto::SceneStaticMesh& proto_buffer) {
    return StreamStorageSingleton::GetInstance().ConnectToNoneStreamBufferInterface(
        proto_buffer.name());
}

StreamInterface<float>* ParseNoneUniformFloatStream(const proto::Uniform& proto_uniform) {
    return StreamStorageSingleton::GetInstance().ConnectToNoneStreamUniformFloatInterface(
        proto_uniform.name());
}

StreamInterface<std::int32_t>* ParseNoneUniformIntStream(const proto::Uniform& proto_uniform) {
    return StreamStorageSingleton::GetInstance().ConnectToNoneStreamUniformIntInterface(
        proto_uniform.name());
}

std::vector<frame::StreamInterface<float>*> ParseAllUniformFloatStream(
    const proto::Uniform& proto_uniform) {
    return StreamStorageSingleton::GetInstance().ConnectToAllStreamUniformFloatInterface(
        proto_uniform.name());
}

std::vector<frame::StreamInterface<std::int32_t>*> ParseAllUniformIntStream(
    const proto::Uniform& proto_uniform) {
    return StreamStorageSingleton::GetInstance().ConnectToAllStreamUniformIntInterface(
        proto_uniform.name());
}

}  // End namespace frame::proto.
