#include "frame/json/parse_level.h"

#include <fstream>

#include "frame/json/parse_json.h"
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

proto::Level LoadLevelProto(const std::string& content)
{
    return LoadProtoFromJson<proto::Level>(content);
}

proto::Level LoadLevelProto(const std::filesystem::path& path)
{
    std::ifstream ifs(path.string().c_str());
    std::string content(std::istreambuf_iterator<char>(ifs), {});
    return LoadLevelProto(content);
}

} // namespace frame::json
