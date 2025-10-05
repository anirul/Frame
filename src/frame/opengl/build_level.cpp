#include "frame/opengl/build_level.h"

#include <format>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/json/parse_material.h"
#include "frame/json/parse_program.h"
#include "frame/json/parse_scene_tree.h"
#include "frame/json/parse_texture.h"

namespace frame::opengl
{

namespace
{

std::unique_ptr<frame::LevelInterface> BuildLevelInternal(
    glm::uvec2 size,
    const frame::proto::Level& proto_level)
{
    auto logger = Logger::GetInstance();
    auto level = std::make_unique<frame::Level>();
    level->SetName(proto_level.name());
    level->SetDefaultTextureName(proto_level.default_texture_name());

    auto cube_id = opengl::CreateCubeStaticMesh(*level.get());
    if (cube_id == NullId)
    {
        throw std::runtime_error("Could not create static cube mesh.");
    }
    level->SetDefaultStaticMeshCubeId(cube_id);

    auto quad_id = opengl::CreateQuadStaticMesh(*level.get());
    if (quad_id == NullId)
    {
        throw std::runtime_error("Could not create static quad mesh.");
    }
    level->SetDefaultStaticMeshQuadId(quad_id);

    for (const auto& proto_texture : proto_level.textures())
    {
        std::unique_ptr<TextureInterface> texture =
            frame::json::ParseTexture(proto_texture, size);
        if (!texture)
        {
            throw std::runtime_error(std::format(
                "Could not load texture: {}", proto_texture.file_name()));
        }
        texture->SetName(proto_texture.name());
        const EntityId texture_id = level->AddTexture(std::move(texture));
        logger->info(std::format(
            "Add a new texture {}, with id [{}].",
            proto_texture.name(),
            texture_id));
        if (!texture_id)
        {
            throw std::runtime_error(std::format(
                "Couldn't save texture {} to level.",
                proto_texture.name()));
        }
    }

    if (!level->GetDefaultOutputTextureId())
    {
        throw std::runtime_error("should have a default texture.");
    }

    for (const auto& proto_program : proto_level.programs())
    {
        auto program = frame::json::ParseProgramOpenGL(proto_program, *level.get());
        if (!program)
        {
            throw std::runtime_error(
                std::format("invalid program: {}", proto_program.name()));
        }
        program->SetName(proto_program.name());
        if (!level->AddProgram(std::move(program)))
        {
            throw std::runtime_error(std::format(
                "Couldn't save program {} to level.", proto_program.name()));
        }
    }

    for (const auto& proto_material : proto_level.materials())
    {
        auto material = frame::json::ParseMaterialOpenGL(proto_material, *level.get());
        if (!material)
        {
            throw std::runtime_error(
                std::format("invalid material : {}", proto_material.name()));
        }
        if (!level->AddMaterial(std::move(material)))
        {
            throw std::runtime_error(std::format(
                "Couldn't save material {} to level.", proto_material.name()));
        }
    }

    if (!frame::json::ParseSceneTreeFile(proto_level.scene_tree(), *level.get()))
    {
        throw std::runtime_error("Could not parse proto scene file.");
    }
    level->SetDefaultCameraName(proto_level.scene_tree().default_camera_name());
    return level;
}

} // namespace

std::unique_ptr<frame::LevelInterface> BuildLevelFromProto(
    glm::uvec2 size,
    const frame::proto::Level& proto_level)
{
    return BuildLevelInternal(size, proto_level);
}

} // namespace frame::opengl
