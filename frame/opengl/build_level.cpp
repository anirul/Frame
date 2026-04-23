#include "frame/opengl/build_level.h"

#include <format>
#include <optional>
#include <stdexcept>

#include "frame/level.h"
#include "frame/logger.h"
#include "frame/json/program_key.h"
#include "frame/node_mesh.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/json/parse_program.h"
#include "frame/opengl/json/parse_scene_tree.h"
#include "frame/opengl/json/parse_texture.h"

namespace frame::opengl
{

namespace
{

bool IsRaytracingProgram(const ProgramInterface& program)
{
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

bool IsRaytracingSourceMaterial(
    frame::LevelInterface& level, frame::EntityId material_id)
{
    if (material_id == frame::NullId)
    {
        return false;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = material.GetProgramId(&level);
    if (program_id == frame::NullId)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    if (!IsRaytracingProgram(program))
    {
        return false;
    }
    return material.GetPreprocessProgramId(&level) != frame::NullId;
}

std::vector<std::pair<frame::EntityId, frame::EntityId>>
GetRaytracingSourceMeshMaterials(frame::LevelInterface& level)
{
    std::vector<std::pair<frame::EntityId, frame::EntityId>> pairs = {};
    const auto append_pairs =
        [&](frame::proto::NodeMesh::RenderTimeEnum render_time_enum) {
            for (const auto& pair : level.GetMeshMaterialIds(render_time_enum))
            {
                if (IsRaytracingSourceMaterial(level, pair.second))
                {
                    pairs.push_back(pair);
                }
            }
        };
    append_pairs(frame::proto::NodeMesh::PRE_RENDER_TIME);
    append_pairs(frame::proto::NodeMesh::SCENE_RENDER_TIME);
    return pairs;
}

bool RaytraceSceneRequiresWorldSpaceBuffers(frame::LevelInterface& level)
{
    for (const auto& [node_id, material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)material_id;
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        auto* skinned_mesh =
            dynamic_cast<SkinnedMesh*>(&level.GetMeshFromId(mesh_id));
        if (!skinned_mesh)
        {
            continue;
        }
        if ((skinned_mesh->HasActiveSkinning() ||
             skinned_mesh->HasActiveRaytraceTriangleCallback() ||
             skinned_mesh->HasActiveRaytraceBvhCallback()) &&
            !skinned_mesh->SupportsGpuRaytraceSkinning())
        {
            return true;
        }
    }
    return false;
}

bool HasRaytracingSourceMeshes(frame::LevelInterface& level)
{
    return !GetRaytracingSourceMeshMaterials(level).empty();
}

bool CanUseSharedRaytraceSceneTransform(
    frame::LevelInterface& level,
    double time_seconds)
{
    const auto source_mesh_materials = GetRaytracingSourceMeshMaterials(level);
    if (source_mesh_materials.empty())
    {
        return false;
    }

    std::optional<glm::mat4> shared_model = std::nullopt;
    for (const auto& [source_node_id, source_material_id] : source_mesh_materials)
    {
        (void)source_material_id;
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(source_node_id));
        if (!node)
        {
            return false;
        }
        const glm::mat4 model = node->GetLocalModel(time_seconds);
        if (!shared_model)
        {
            shared_model = model;
            continue;
        }
        if (*shared_model != model)
        {
            return false;
        }
    }
    return true;
}

bool HasDynamicRaytracingSourceMesh(frame::LevelInterface& level)
{
    for (const auto& [node_id, material_id] :
         GetRaytracingSourceMeshMaterials(level))
    {
        (void)material_id;
        auto* node =
            dynamic_cast<NodeMesh*>(&level.GetSceneNodeFromId(node_id));
        if (!node)
        {
            continue;
        }
        const auto mesh_id = node->GetLocalMesh();
        if (!mesh_id)
        {
            continue;
        }
        auto* skinned_mesh =
            dynamic_cast<SkinnedMesh*>(&level.GetMeshFromId(mesh_id));
        if (!skinned_mesh)
        {
            continue;
        }
        if (skinned_mesh->HasActiveSkinning() ||
            skinned_mesh->HasActiveRaytraceTriangleCallback() ||
            skinned_mesh->HasActiveRaytraceBvhCallback())
        {
            return true;
        }
    }
    return false;
}

bool RequiresInstancedRaytraceProgram(frame::LevelInterface& level)
{
    if (!HasRaytracingSourceMeshes(level))
    {
        return false;
    }
    if (RaytraceSceneRequiresWorldSpaceBuffers(level))
    {
        return false;
    }
    if (HasDynamicRaytracingSourceMesh(level))
    {
        return true;
    }
    return !CanUseSharedRaytraceSceneTransform(level, 0.0);
}

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

void EnableInstancedRaytraceProgramIfNeeded(
    frame::LevelInterface& level,
    const frame::json::LevelData& level_data)
{
    if (!RequiresInstancedRaytraceProgram(level))
    {
        return;
    }

    const auto shared_program_id = level.GetIdFromName("RayTraceProgram");
    if (!shared_program_id)
    {
        throw std::runtime_error("Missing shared OpenGL raytrace program.");
    }

    const auto* shared_program_info = [&]() -> const frame::json::ProgramInfo* {
        for (const auto& program_info : level_data.programs)
        {
            if (program_info.name == "RayTraceProgram")
            {
                return &program_info;
            }
        }
        return nullptr;
    }();
    if (!shared_program_info)
    {
        throw std::runtime_error(
            "Missing embedded RayTraceProgram definition for OpenGL.");
    }

    auto proto_program = shared_program_info->proto;
    proto_program.set_name("RayTraceProgramInstanced");
    auto shader_files = shared_program_info->opengl;
    shader_files.fragment_shader = "raytrace_instanced.frag";
    auto instanced_program =
        frame::json::ParseProgramOpenGL(proto_program, shader_files, level);
    if (!instanced_program)
    {
        throw std::runtime_error(
            "Could not build OpenGL instanced raytrace program.");
    }
    instanced_program->SetName("RayTraceProgramInstanced");
    const auto instanced_program_id = level.AddProgram(std::move(instanced_program));
    if (!instanced_program_id)
    {
        throw std::runtime_error(
            "Could not store OpenGL instanced raytrace program.");
    }

    const auto preprocess_id = level.GetRenderPassPreprocessProgramId(
        proto::NodeMesh::SCENE_RENDER_TIME);
    level.SetRenderPassProgramIds(
        proto::NodeMesh::SCENE_RENDER_TIME,
        instanced_program_id,
        preprocess_id);

    for (const auto material_id : level.GetMaterials())
    {
        auto& material = level.GetMaterialFromId(material_id);
        if (material.GetProgramId(&level) != shared_program_id ||
            material.GetPreprocessProgramId(&level) != NullId)
        {
            continue;
        }
        material.SetProgramId(instanced_program_id);
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
    EnableInstancedRaytraceProgramIfNeeded(*level, level_data);
    return level;
}

} // namespace frame::opengl
