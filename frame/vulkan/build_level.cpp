#include "frame/vulkan/build_level.h"

#include <format>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/vulkan/json/parse_material.h"
#include "frame/vulkan/json/parse_program.h"
#include "frame/vulkan/json/parse_scene_tree.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan
{

BuiltLevel BuildLevel(
    glm::uvec2 size,
    const frame::json::LevelData& level_data)
{
    BuiltLevel built;
    auto level = std::make_unique<frame::Level>();
    level->SetName(level_data.proto.name());
    level->SetDefaultTextureName(level_data.proto.default_texture_name());

    auto cube_id = CreateCubeStaticMesh(*level);
    if (!cube_id)
    {
        throw std::runtime_error("Failed to create default cube mesh for Vulkan level.");
    }
    level->SetDefaultStaticMeshCubeId(cube_id);

    auto quad_id = CreateQuadStaticMesh(*level);
    if (!quad_id)
    {
        throw std::runtime_error("Failed to create default quad mesh for Vulkan level.");
    }
    level->SetDefaultStaticMeshQuadId(quad_id);

    for (const auto& proto_texture : level_data.proto.textures())
    {
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

    if (!level->GetDefaultOutputTextureId())
    {
        throw std::runtime_error("Default output texture is missing.");
    }

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

    if (!json::ParseSceneTree(level_data.proto.scene_tree(), *level))
    {
        throw std::runtime_error("Failed to parse scene tree for Vulkan level.");
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
