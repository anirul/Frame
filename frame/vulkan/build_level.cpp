#include "frame/vulkan/build_level.h"

#include <chrono>
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

namespace
{

class ScopedTimer
{
  public:
    ScopedTimer(const Logger& logger, std::string label)
        : logger_(logger),
          label_(std::move(label)),
          start_(Clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - start_);
        logger_->info("{} took {} ms.", label_, elapsed.count());
    }

  private:
    using Clock = std::chrono::steady_clock;
    const Logger& logger_;
    std::string label_;
    Clock::time_point start_;
};

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
        level->SetDefaultStaticMeshCubeId(cube_id);

        auto quad_id = CreateQuadStaticMesh(*level);
        if (!quad_id)
        {
            throw std::runtime_error("Failed to create default quad mesh for Vulkan level.");
        }
        level->SetDefaultStaticMeshQuadId(quad_id);
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
