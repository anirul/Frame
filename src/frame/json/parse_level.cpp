#include "frame/json/parse_level.h"

#include "frame/json/parse_json.h"
#include "frame/json/parse_material.h"
#include "frame/json/parse_program.h"
#include "frame/json/parse_scene_tree.h"
#include "frame/json/parse_texture.h"
#include "frame/level.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/program_interface.h"

namespace frame::proto
{

namespace
{

std::unique_ptr<LevelInterface> LevelProto(
    glm::uvec2 size, const proto::Level &proto_level)
{
    // TODO(anirul): Check we are in OPENGL mode?
    auto logger = Logger::GetInstance();
    auto level = std::make_unique<frame::Level>();
    level->SetName(proto_level.name());
    level->SetDefaultTextureName(proto_level.default_texture_name());

    // Include the default cube and quad.
    auto cube_id = opengl::CreateCubeStaticMesh(*level.get());
    if (cube_id == NullId)
        throw std::runtime_error("Could not create static cube mesh.");
    level->SetDefaultStaticMeshCubeId(cube_id);
    auto quad_id = opengl::CreateQuadStaticMesh(*level.get());
    if (quad_id == NullId)
        throw std::runtime_error("Could not create static quad mesh.");
    level->SetDefaultStaticMeshQuadId(quad_id);

    // Load textures from proto.
    for (const auto &proto_texture : proto_level.textures())
    {
        std::unique_ptr<TextureInterface> texture =
            ParseBasicTexture(proto_texture, size);
        EntityId stream_id = NullId;
        EntityId texture_id = NullId;
        std::string texture_name = proto_texture.name();
        if (!texture)
        {
            throw std::runtime_error(fmt::format(
                "Could not load texture: {}", proto_texture.file_name()));
        }
        texture->SetName(texture_name);
        texture_id = level->AddTexture(std::move(texture));
        logger->info(fmt::format(
            "Add a new texture {}, with id [{}].",
            proto_texture.name(),
            texture_id));
        if (!texture_id)
        {
            throw std::runtime_error(fmt::format(
                "Coudn't save texture {} to level.", proto_texture.name()));
        }
    }

    if (!level->GetDefaultOutputTextureId())
    {
        throw std::runtime_error("should have a default texture.");
    }

    // Load programs from proto.
    for (const auto &proto_program : proto_level.programs())
    {
        auto program = ParseProgramOpenGL(proto_program, *level.get());
        if (!program)
        {
            throw std::runtime_error(
                fmt::format("invalid program: {}", proto_program.name()));
        }
        program->SetName(proto_program.name());
        if (!level->AddProgram(std::move(program)))
        {
            throw std::runtime_error(fmt::format(
                "Couldn't save program {} to level.", proto_program.name()));
        }
    }

    // Load material from proto.
    for (const auto &proto_material : proto_level.materials())
    {
        auto maybe_material = ParseMaterialOpenGL(proto_material, *level.get());
        if (!maybe_material)
        {
            throw std::runtime_error(
                fmt::format("invalid material : {}", proto_material.name()));
        }
        auto material = std::move(maybe_material.value());
        if (!level->AddMaterial(std::move(material)))
        {
            throw std::runtime_error(fmt::format(
                "Couldn't save material {} to level.", proto_material.name()));
        }
    }

    // Load scenes from proto.
    if (!ParseSceneTreeFile(proto_level.scene_tree(), *level.get()))
    {
        throw std::runtime_error("Could not parse proto scene file.");
    }
    level->SetDefaultCameraName(proto_level.scene_tree().default_camera_name());
    return level;
}

} // End namespace.

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const proto::Level &proto_level)
{
    return LevelProto(size, proto_level);
}

std::unique_ptr<frame::LevelInterface> ParseLevel(
    glm::uvec2 size, const std::string &content)
{
    auto proto_level = LoadProtoFromJson<Level>(content);
    return LevelProto(size, proto_level);
}

std::unique_ptr<LevelInterface> ParseLevel(
    glm::uvec2 size, const std::filesystem::path &path)
{
    std::ifstream ifs(path.string().c_str());
    std::string content(std::istreambuf_iterator<char>(ifs), {});
    auto proto_level = LoadProtoFromJson<Level>(content);
    return LevelProto(size, proto_level);
}

} // End namespace frame::proto.
