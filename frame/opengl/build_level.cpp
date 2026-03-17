#include "frame/opengl/build_level.h"

#include <format>
#include <stdexcept>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/json/parse_program.h"
#include "frame/opengl/json/parse_scene_tree.h"
#include "frame/opengl/json/parse_texture.h"

namespace frame::opengl
{

namespace
{

void ConfigureRenderPassPrograms(
    frame::LevelInterface& level,
    const frame::json::LevelData& level_data)
{
    for (const auto& pass : level_data.render_pass_programs)
    {
        const auto program_id = level.GetIdFromName(pass.program_name);
        if (!program_id)
        {
            throw std::runtime_error(std::format(
                "Unknown internal program '{}' for render pass {}.",
                pass.program_name,
                static_cast<int>(pass.render_time)));
        }
        EntityId preprocess_id = NullId;
        if (!pass.preprocess_program_name.empty())
        {
            preprocess_id = level.GetIdFromName(pass.preprocess_program_name);
            if (!preprocess_id)
            {
                throw std::runtime_error(std::format(
                    "Unknown internal preprocess program '{}' for render pass {}.",
                    pass.preprocess_program_name,
                    static_cast<int>(pass.render_time)));
            }
        }
        level.SetRenderPassProgramIds(
            pass.render_time,
            program_id,
            preprocess_id);
    }
}

} // namespace

std::unique_ptr<frame::LevelInterface> BuildLevel(
    glm::uvec2 size,
    const frame::json::LevelData& level_data)
{
    auto logger = Logger::GetInstance();
    auto level = std::make_unique<frame::Level>();
    level->SetName(level_data.proto.name());
    level->SetDefaultTextureName(level_data.proto.default_texture_name());

    auto cube_id = opengl::CreateCubeMesh(*level);
    if (cube_id == NullId)
    {
        throw std::runtime_error("Could not create cube mesh.");
    }
    level->SetDefaultMeshCubeId(cube_id);

    auto quad_id = opengl::CreateQuadMesh(*level);
    if (quad_id == NullId)
    {
        throw std::runtime_error("Could not create quad mesh.");
    }
    level->SetDefaultMeshQuadId(quad_id);

    for (const auto& proto_texture : level_data.proto.textures())
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

    for (const auto& program_info : level_data.programs)
    {
        auto program = frame::json::ParseProgramOpenGL(
            program_info.proto,
            program_info.opengl,
            *level);
        if (!program)
        {
            throw std::runtime_error(std::format(
                "invalid internal program: {}",
                program_info.name));
        }
        program->SetName(program_info.name);
        if (!level->AddProgram(std::move(program)))
        {
            throw std::runtime_error(std::format(
                "Couldn't save program {} to level.", program_info.name));
        }
    }

    ConfigureRenderPassPrograms(*level, level_data);

    if (!frame::json::ParseSceneTreeFile(level_data.proto.scene_tree(), *level))
    {
        throw std::runtime_error("Could not parse proto scene file.");
    }
    if (level_data.proto.has_scene_tree())
    {
        level->SetDefaultCameraName(
            level_data.proto.scene_tree().default_camera_name());
        level->SetDefaultRootSceneNodeName(
            level_data.proto.scene_tree().default_root_name());
    }
    return level;
}

} // namespace frame::opengl
