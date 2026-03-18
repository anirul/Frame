#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "frame/json/proto.h"

namespace frame::json
{

struct TextureInfo
{
    std::string name;
    proto::PixelElementSize element_size;
    proto::PixelStructure structure;
    glm::uvec2 size{};
};

struct ShaderFiles
{
    std::string vertex_shader;
    std::string fragment_shader;
    std::string compute_shader;
    std::string raygen_shader;
    std::string miss_shader;
    std::string closesthit_shader;
};

struct ProgramInfo
{
    std::string name;
    proto::Program proto;
    ShaderFiles opengl;
    ShaderFiles vulkan;
};

struct RenderPassProgramInfo
{
    proto::NodeMesh::RenderTimeEnum render_time =
        proto::NodeMesh::SCENE_RENDER_TIME;
    std::string program_name;
    std::string preprocess_program_name;
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
    std::filesystem::path source_path;
    std::vector<TextureInfo> textures;
    std::vector<ProgramInfo> programs;
    std::vector<RenderPassProgramInfo> render_pass_programs;
    std::vector<StaticMeshInfo> meshes;
};

LevelData BuildLevelData(
    glm::uvec2 size,
    const proto::Level& proto_level,
    const std::filesystem::path& asset_root,
    const std::filesystem::path& source_path = {});

} // namespace frame::json
