#include "frame/json/parse_level.h"

#include "frame/file/file_system.h"
#include "frame/opengl/build_level.h"

namespace frame::json
{

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const proto::Level& proto_level)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    return frame::opengl::BuildLevel(
        size,
        ParseLevelData(size, proto_level, asset_root));
}

std::unique_ptr<frame::LevelInterface> ParseLevel(
    glm::uvec2 size, const std::string& content)
{
    return ParseLevel(size, LoadLevelProto(content));
}

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const std::filesystem::path& path)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    return frame::opengl::BuildLevel(
        size,
        ParseLevelData(size, path, asset_root));
}

} // namespace frame::json
