#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "frame/proto/level.pb.h"

namespace frame::json
{

struct TextureInfo
{
    std::string name;
    proto::PixelElementSize element_size;
    proto::PixelStructure structure;
    glm::uvec2 size{};
};

struct ProgramInfo
{
    std::string name;
    std::string vertex_shader;
    std::string fragment_shader;
};

struct MaterialInfo
{
    std::string name;
    std::string program_name;
    std::vector<std::string> texture_names;
};

struct StaticMeshInfo
{
    std::string name;
    std::vector<float> positions;
    std::vector<float> uvs;
    std::vector<std::uint32_t> indices;
};

struct LevelData
{
    proto::Level proto;
    std::filesystem::path asset_root;
    std::vector<TextureInfo> textures;
    std::vector<ProgramInfo> programs;
    std::vector<MaterialInfo> materials;
    std::vector<StaticMeshInfo> meshes;
};

LevelData BuildLevelData(
    glm::uvec2 size,
    const proto::Level& proto_level,
    const std::filesystem::path& asset_root);

} // namespace frame::json
