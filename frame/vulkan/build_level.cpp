#include "frame/vulkan/build_level.h"

#include <format>
#include <stdexcept>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/vulkan/scoped_timer.h"
#include "frame/vulkan/json/parse_program.h"
#include "frame/vulkan/json/parse_scene_tree.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan
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
                "Unknown internal program '{}' for Vulkan render pass {}.",
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
                    "Unknown internal preprocess program '{}' for Vulkan render pass {}.",
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

BuiltLevel BuildLevel(
    glm::uvec2 size,
    const frame::json::LevelData& level_data,
    const BuildLevelOptions& options)
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
        ScopedTimer timer(logger, "Parse internal programs");
        for (const auto& program_info : level_data.programs)
        {
            auto program = json::ParseProgram(program_info.proto, *level);
            program->SetName(program_info.name);
            if (!level->AddProgram(std::move(program)))
            {
                throw std::runtime_error(std::format(
                    "Unable to add internal program {} to Vulkan level.",
                    program_info.name));
            }
        }
    }

    ConfigureRenderPassPrograms(*level, level_data);

    {
        ScopedTimer timer(logger, "Parse scene tree");
        if (!json::ParseSceneTree(
                level_data.proto.scene_tree(),
                *level,
                {.prefer_hardware_raytracing =
                     options.prefer_hardware_raytracing}))
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
