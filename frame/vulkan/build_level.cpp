#include "frame/vulkan/build_level.h"

#include <format>
#include <string>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/json/program_catalog.h"
#include "frame/vulkan/scoped_timer.h"
#include "frame/vulkan/json/parse_material.h"
#include "frame/vulkan/json/parse_program.h"
#include "frame/vulkan/json/parse_scene_tree.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan
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

} // namespace

BuiltLevel BuildLevel(
    glm::uvec2 size,
    const frame::json::LevelData& level_data)
{
    auto& logger = frame::Logger::GetInstance();
    ScopedTimer total_timer(logger, "Vulkan BuildLevel");

    BuiltLevel built;
    auto level = std::make_unique<frame::Level>();
    level->SetName(level_data.proto.name());
    level->SetDefaultTextureName(level_data.proto.default_texture_name());

    {
        ScopedTimer timer(logger, "Create default meshes");
        auto cube_id = CreateCubeStaticMesh(*level);
        if (!cube_id)
        {
            throw std::runtime_error("Failed to create default cube mesh for Vulkan level.");
        }
        level->SetDefaultMeshCubeId(cube_id);

        auto quad_id = CreateQuadStaticMesh(*level);
        if (!quad_id)
        {
            throw std::runtime_error("Failed to create default quad mesh for Vulkan level.");
        }
        level->SetDefaultMeshQuadId(quad_id);
    }

    {
        ScopedTimer timer(logger, "Parse textures");
        for (const auto& proto_texture : level_data.proto.textures())
        {
            ScopedTimer texture_timer(
                logger,
                std::string("Parse texture ") + proto_texture.name());
            auto texture = json::ParseTexture(proto_texture, size);
            texture->SetName(proto_texture.name());
            auto texture_id = level->AddTexture(std::move(texture));
            if (!texture_id)
            {
                throw std::runtime_error(std::format(
                    "Unable to add texture {} to Vulkan level.",
                    proto_texture.name()));
            }
        }
    }

    if (!level->GetDefaultOutputTextureId())
    {
        throw std::runtime_error("Default output texture is missing.");
    }

    {
        ScopedTimer timer(logger, "Parse programs");
        for (const auto& proto_program : level_data.proto.programs())
        {
            auto program = json::ParseProgram(proto_program, *level);
            program->SetName(proto_program.name());
            if (!level->AddProgram(std::move(program)))
            {
                throw std::runtime_error(std::format(
                    "Unable to add program {} to Vulkan level.",
                    proto_program.name()));
            }
        }
    }

    {
        ScopedTimer timer(logger, "Parse materials");
        for (const auto& proto_material : level_data.proto.materials())
        {
            auto material = json::ParseMaterial(proto_material, *level);
            if (!material)
            {
                throw std::runtime_error(std::format(
                    "Invalid material {} while building Vulkan level.",
                    proto_material.name()));
            }
            if (!level->AddMaterial(std::move(material)))
            {
                throw std::runtime_error(std::format(
                    "Unable to add material {} to Vulkan level.",
                    proto_material.name()));
            }
        }
    }

    ConfigureRenderPassPrograms(*level, level_data.proto);

    {
        ScopedTimer timer(logger, "Parse scene tree");
        if (!json::ParseSceneTree(level_data.proto.scene_tree(), *level))
        {
            throw std::runtime_error("Failed to parse scene tree for Vulkan level.");
        }
    }

    if (level_data.proto.has_scene_tree())
    {
        level->SetDefaultCameraName(
            level_data.proto.scene_tree().default_camera_name());
        level->SetDefaultRootSceneNodeName(
            level_data.proto.scene_tree().default_root_name());
    }
    built.level = std::move(level);
    return built;
}

} // namespace frame::vulkan

