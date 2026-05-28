#include "frame/json/parse_level.h"

#include <fstream>

#include "frame/json/level_data.h"
#include "frame/json/parse_json.h"

namespace frame::json
{

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

LevelData ParseLevelData(
    glm::uvec2 size,
    const proto::Level& proto,
    const std::filesystem::path& asset_root)
{
    return BuildLevelData(size, proto, asset_root);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const proto::Level& proto,
    const std::filesystem::path& asset_root,
    const LevelDataOptions& options)
{
    return BuildLevelData(size, proto, asset_root, {}, options);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const proto::Level& proto,
    const std::filesystem::path& asset_root,
    const std::filesystem::path& source_path)
{
    return BuildLevelData(size, proto, asset_root, source_path);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const proto::Level& proto,
    const std::filesystem::path& asset_root,
    const std::filesystem::path& source_path,
    const LevelDataOptions& options)
{
    return BuildLevelData(size, proto, asset_root, source_path, options);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const std::string& content,
    const std::filesystem::path& asset_root)
{
    return ParseLevelData(size, LoadLevelProto(content), asset_root);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const std::string& content,
    const std::filesystem::path& asset_root,
    const LevelDataOptions& options)
{
    return ParseLevelData(size, LoadLevelProto(content), asset_root, options);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const std::filesystem::path& path,
    const std::filesystem::path& asset_root)
{
    return ParseLevelData(size, LoadLevelProto(path), asset_root, path);
}

LevelData ParseLevelData(
    glm::uvec2 size,
    const std::filesystem::path& path,
    const std::filesystem::path& asset_root,
    const LevelDataOptions& options)
{
    return ParseLevelData(
        size, LoadLevelProto(path), asset_root, path, options);
}

} // namespace frame::json
