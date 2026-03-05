#include "frame/opengl/build_level.h"

#include <format>
#include <string>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/json/program_catalog.h"
#include "frame/opengl/material.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/texture.h"
#include "frame/opengl/json/parse_material.h"
#include "frame/opengl/json/parse_program.h"
#include "frame/opengl/json/parse_scene_tree.h"
#include "frame/opengl/json/parse_texture.h"

namespace frame::opengl
{

namespace
{

EntityId FindProgramBySceneType(
    const frame::LevelInterface& level,
    frame::proto::SceneType::Enum scene_type)
{
    for (const auto program_id : level.GetPrograms())
    {
        const auto& program = level.GetProgramFromId(program_id);
        if (program.GetData().input_scene_type().value() == scene_type)
        {
            return program_id;
        }
    }
    return NullId;
}

EntityId FindRaytracingBvhProgramBySceneType(
    const frame::LevelInterface& level,
    frame::proto::SceneType::Enum scene_type)
{
    for (const auto program_id : level.GetPrograms())
    {
        const auto& program = level.GetProgramFromId(program_id);
        if (program.GetData().input_scene_type().value() != scene_type)
        {
            continue;
        }
        const auto key = frame::json::ResolveProgramKey(program.GetData());
        if (frame::json::IsRaytracingBvhProgramKey(key))
        {
            return program_id;
        }
    }
    return NullId;
}

EntityId FindRaytracingPreprocessProgram(
    const frame::LevelInterface& level)
{
    for (const auto program_id : level.GetPrograms())
    {
        const auto& program = level.GetProgramFromId(program_id);
        const auto key = frame::json::ResolveProgramKey(program.GetData());
        if (frame::json::IsRaytracingProgramKey(key) &&
            key.find("preprocess") != std::string::npos)
        {
            return program_id;
        }
    }
    return NullId;
}

void ConfigureRenderPassPrograms(
    frame::LevelInterface& level,
    const frame::proto::Level& proto_level)
{
    bool has_explicit_config = false;
    for (const auto& pass : proto_level.render_pass_programs())
    {
        const auto program_id = level.GetIdFromName(pass.program_name());
        if (!program_id)
        {
            throw std::runtime_error(std::format(
                "Unknown program '{}' in render_pass_programs.",
                pass.program_name()));
        }
        EntityId preprocess_id = NullId;
        if (pass.has_preprocess_program_name() &&
            !pass.preprocess_program_name().empty())
        {
            preprocess_id =
                level.GetIdFromName(pass.preprocess_program_name());
            if (!preprocess_id)
            {
                throw std::runtime_error(std::format(
                    "Unknown preprocess program '{}' in render_pass_programs.",
                    pass.preprocess_program_name()));
            }
        }
        level.SetRenderPassProgramIds(
            pass.render_time_enum(),
            program_id,
            preprocess_id);
        has_explicit_config = true;
    }
    if (has_explicit_config)
    {
        return;
    }

    const auto skybox_program_id = FindProgramBySceneType(
        level,
        frame::proto::SceneType::CUBE);
    if (skybox_program_id)
    {
        level.SetRenderPassProgramIds(
            frame::proto::NodeMesh::SKYBOX_RENDER_TIME,
            skybox_program_id);
    }

    EntityId scene_program_id = FindRaytracingBvhProgramBySceneType(
        level,
        frame::proto::SceneType::QUAD);
    if (!scene_program_id)
    {
        scene_program_id = FindRaytracingBvhProgramBySceneType(
            level,
            frame::proto::SceneType::SCENE);
    }
    if (!scene_program_id)
    {
        scene_program_id = FindProgramBySceneType(
            level,
            frame::proto::SceneType::SCENE);
    }
    if (!scene_program_id)
    {
        scene_program_id = FindProgramBySceneType(
            level,
            frame::proto::SceneType::QUAD);
    }
    if (scene_program_id)
    {
        EntityId preprocess_id = NullId;
        const auto& scene_program = level.GetProgramFromId(scene_program_id);
        const auto key = frame::json::ResolveProgramKey(scene_program.GetData());
        if (frame::json::IsRaytracingBvhProgramKey(key))
        {
            preprocess_id = FindRaytracingPreprocessProgram(level);
        }
        level.SetRenderPassProgramIds(
            frame::proto::NodeMesh::SCENE_RENDER_TIME,
            scene_program_id);
        level.SetRenderPassProgramIds(
            frame::proto::NodeMesh::PRE_RENDER_TIME,
            scene_program_id,
            preprocess_id);
        level.SetRenderPassProgramIds(
            frame::proto::NodeMesh::POST_PROCESS_TIME,
            scene_program_id);
    }
}

std::unique_ptr<frame::LevelInterface> BuildLevelInternal(
    glm::uvec2 size,
    const frame::proto::Level& proto_level)
{
    auto logger = Logger::GetInstance();
    auto level = std::make_unique<frame::Level>();
    level->SetName(proto_level.name());
    level->SetDefaultTextureName(proto_level.default_texture_name());

    auto cube_id = opengl::CreateCubeMesh(*level.get());
    if (cube_id == NullId)
    {
        throw std::runtime_error("Could not create cube mesh.");
    }
    level->SetDefaultMeshCubeId(cube_id);

    auto quad_id = opengl::CreateQuadMesh(*level.get());
    if (quad_id == NullId)
    {
        throw std::runtime_error("Could not create quad mesh.");
    }
    level->SetDefaultMeshQuadId(quad_id);

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

    ConfigureRenderPassPrograms(*level, proto_level);

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


