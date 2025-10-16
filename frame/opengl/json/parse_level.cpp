#include "frame/json/parse_level.h"

#include "frame/opengl/build_level.h"

namespace frame::json
{

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const proto::Level& proto_level)
{
    return frame::opengl::BuildLevelFromProto(size, proto_level);
}

std::unique_ptr<frame::LevelInterface> ParseLevel(
    glm::uvec2 size, const std::string& content)
{
    return ParseLevel(size, LoadLevelProto(content));
}

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const std::filesystem::path& path)
{
    return ParseLevel(size, LoadLevelProto(path));
}

} // namespace frame::json
