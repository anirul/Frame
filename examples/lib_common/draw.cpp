#include "Draw.h"

#include <fstream>

#include "Frame/File/FileSystem.h"
#include "Frame/Proto/ParseLevel.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size) {
    // Load proto from files.
    auto proto_level = frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(path_.string());
    // Load level from proto files.
    auto maybe_level = frame::proto::ParseLevelOpenGL(size_, proto_level);
    if (!maybe_level) throw std::runtime_error("Couldn't load level!");
    device_->Startup(std::move(maybe_level.value()));
}

void Draw::RunDraw(const double dt) {}
