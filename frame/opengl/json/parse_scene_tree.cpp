#include "frame/opengl/json/parse_scene_tree.h"

#include <array>
#include <cctype>
#include <format>
#include <optional>
#include <unordered_set>

#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/program_catalog.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/file/load_mesh.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/mesh.h"

namespace frame::json
{

namespace
{

std::function<NodeInterface*(const std::string& name)> GetFunctor(
    LevelInterface& level)
{
    return [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
        {
            throw std::runtime_error(std::format("No id from name: {}", name));
        }
        EntityId id = maybe_id;
        return &level.GetSceneNodeFromId(id);
    };
}

bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        const auto l = static_cast<unsigned char>(lhs[i]);
        const auto r = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(l) != std::tolower(r))
        {
            return false;
        }
    }
    return true;
}

std::optional<std::string> ResolveSamplerNameForTexture(
    const ProgramInterface& program,
    const std::string& texture_name)
{
    std::vector<std::string> sampler_names = {};
    for (const auto& binding : program.GetData().bindings())
    {
        if (binding.binding_type() ==
            frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER)
        {
            sampler_names.push_back(binding.name());
        }
    }
    if (!sampler_names.empty())
    {
        for (const auto& sampler_name : sampler_names)
        {
            if (sampler_name == texture_name)
            {
                return sampler_name;
            }
        }
        for (const auto& sampler_name : sampler_names)
        {
            if (EqualsIgnoreCase(sampler_name, texture_name))
            {
                return sampler_name;
            }
        }
        if (sampler_names.size() == 1)
        {
            return sampler_names.front();
        }
    }
    if (program.HasUniform(texture_name))
    {
        return texture_name;
    }
    return std::nullopt;
}

EntityId FindTextureIdByName(
    LevelInterface& level, const std::string& texture_name)
{
    for (const auto texture_id : level.GetTextures())
    {
        const auto candidate_name = level.GetNameFromId(texture_id);
        if (candidate_name == texture_name ||
            EqualsIgnoreCase(candidate_name, texture_name))
        {
            return texture_id;
        }
    }
    return NullId;
}

void BindMaterialTexturesFromProgram(
    MaterialInterface& material,
    LevelInterface& level,
    const ProgramInterface& program)
{
    if (!material.GetTextureIds().empty())
    {
        return;
    }
    std::unordered_set<EntityId> bound_texture_ids = {};
    for (const auto texture_id : program.GetInputTextureIds())
    {
        if (!texture_id || !bound_texture_ids.insert(texture_id).second)
        {
            continue;
        }
        const auto texture_name = level.GetNameFromId(texture_id);
        const auto inner_name = ResolveSamplerNameForTexture(
                                    program,
                                    texture_name)
                                    .value_or(texture_name);
        material.AddTextureId(texture_id, inner_name);
    }
    for (const auto& binding : program.GetData().bindings())
    {
        if (binding.binding_type() !=
            frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER)
        {
            continue;
        }
        const auto texture_id = FindTextureIdByName(level, binding.name());
        if (!texture_id || !bound_texture_ids.insert(texture_id).second)
        {
            continue;
        }
        material.AddTextureId(texture_id, binding.name());
    }
}

void ConfigureMaterialProgramsForRenderTime(
    LevelInterface& level,
    EntityId material_id,
    proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    if (!material_id)
    {
        return;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (program_id)
    {
        material.SetProgramId(program_id);
        auto& program = level.GetProgramFromId(program_id);
        BindMaterialTexturesFromProgram(material, level, program);
    }
    const auto preprocess_program_id =
        level.GetRenderPassPreprocessProgramId(render_time_enum);
    if (preprocess_program_id)
    {
        material.SetPreprocessProgramId(preprocess_program_id);
    }
}

bool IsRaytracingBvhMaterial(LevelInterface& level, EntityId material_id)
{
    if (!material_id)
    {
        return false;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto program_id = material.GetProgramId(&level);
    if (!program_id)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingBvhProgramKey(key);
}

void ReplaceTextureBindingByInnerName(
    MaterialInterface& material,
    const std::string& inner_name,
    EntityId texture_id)
{
    if (!texture_id)
    {
        return;
    }
    std::vector<EntityId> to_remove = {};
    for (const auto id : material.GetTextureIds())
    {
        if (material.GetInnerName(id) == inner_name)
        {
            to_remove.push_back(id);
        }
    }
    for (const auto id : to_remove)
    {
        material.RemoveTextureId(id);
    }
    material.AddTextureId(texture_id, inner_name);
}

void AdoptGltfPbrTextures(
    LevelInterface& level,
    EntityId source_material_id,
    EntityId target_material_id)
{
    if (!source_material_id ||
        !target_material_id ||
        source_material_id == target_material_id)
    {
        return;
    }
    if (!IsRaytracingBvhMaterial(level, target_material_id))
    {
        return;
    }
    auto& source = level.GetMaterialFromId(source_material_id);
    auto& target = level.GetMaterialFromId(target_material_id);
    const auto find_source_texture = [&](const std::string& inner_name) {
        for (const auto texture_id : source.GetTextureIds())
        {
            if (source.GetInnerName(texture_id) == inner_name)
            {
                return texture_id;
            }
        }
        return NullId;
    };
    const auto find_target_texture = [&](const std::string& inner_name) {
        for (const auto texture_id : target.GetTextureIds())
        {
            if (target.GetInnerName(texture_id) == inner_name)
            {
                return texture_id;
            }
        }
        return NullId;
    };

    std::array<std::pair<const char*, const char*>, 5> pbr_mappings = {{
        {"albedo_texture", "Color"},
        {"normal_texture", nullptr},
        {"roughness_texture", nullptr},
        {"metallic_texture", nullptr},
        {"ao_texture", nullptr},
    }};
    for (const auto& [target_name, fallback_name] : pbr_mappings)
    {
        EntityId source_id = find_source_texture(target_name);
        if (!source_id && fallback_name)
        {
            source_id = find_source_texture(fallback_name);
        }
        if (!source_id)
        {
            continue;
        }
        const auto source_texture_name = level.GetNameFromId(source_id);
        // Only adopt file-backed glTF textures. This avoids overriding
        // explicit level textures with generated solid-color fallbacks.
        if (source_texture_name.find(".__gltf_tex_") == std::string::npos)
        {
            continue;
        }

        const EntityId existing_target_id =
            find_target_texture(target_name);
        if (existing_target_id != NullId)
        {
            const auto existing_target_texture_name =
                level.GetNameFromId(existing_target_id);
            const bool replace_generated_target =
                existing_target_texture_name.find(".__gltf_solid_") !=
                    std::string::npos ||
                existing_target_texture_name.find(".__gltf_tex_") !=
                    std::string::npos;
            if (!replace_generated_target)
            {
                continue;
            }
        }
        ReplaceTextureBindingByInnerName(target, target_name, source_id);
    }
}

void EnsureRaytracingBvhBuffers(
    LevelInterface& level, EntityId material_id, const MeshInterface& mesh)
{
    if (!IsRaytracingBvhMaterial(level, material_id))
    {
        return;
    }
    auto& material = level.GetMaterialFromId(material_id);
    const auto triangle_buffer_id = mesh.GetTriangleBufferId();
    if (!triangle_buffer_id)
    {
        return;
    }
    material.AddBufferName(
        level.GetNameFromId(triangle_buffer_id),
        "TriangleBuffer");

    const auto bvh_buffer_id = mesh.GetBvhBufferId();
    if (!bvh_buffer_id)
    {
        Logger::GetInstance()->warn(
            "Raytracing material '{}' has no BVH buffer bound.",
            material.GetData().name());
        return;
    }
    material.AddBufferName(level.GetNameFromId(bvh_buffer_id), "BvhBuffer");
}

void ApplyAnimationPlayback(
    const proto::NodeMesh& proto_scene_mesh,
    NodeMesh& node,
    MeshInterface* mesh)
{
    node.GetData().set_play_animation(proto_scene_mesh.play_animation());
    if (proto_scene_mesh.has_animation_speed())
    {
        node.GetData().set_animation_speed(
            proto_scene_mesh.animation_speed());
    }
    if (proto_scene_mesh.has_animation_clip_name())
    {
        node.GetData().set_animation_clip_name(
            proto_scene_mesh.animation_clip_name());
    }
    if (proto_scene_mesh.has_animation_clip_index())
    {
        node.GetData().set_animation_clip_index(
            proto_scene_mesh.animation_clip_index());
    }
    if (!mesh)
    {
        return;
    }
    auto* gl_mesh = dynamic_cast<opengl::SkinnedMesh*>(mesh);
    if (!gl_mesh)
    {
        return;
    }
    const float speed = proto_scene_mesh.has_animation_speed()
                            ? proto_scene_mesh.animation_speed()
                            : 1.0f;
    gl_mesh->SetSkinningAnimation(proto_scene_mesh.play_animation(), speed);
    const std::string clip_name = proto_scene_mesh.has_animation_clip_name()
                                      ? proto_scene_mesh.animation_clip_name()
                                      : "";
    std::optional<std::uint32_t> clip_index = std::nullopt;
    if (proto_scene_mesh.has_animation_clip_index())
    {
        clip_index = proto_scene_mesh.animation_clip_index();
    }
    gl_mesh->SetSkinningAnimationClip(clip_name, clip_index);
    if (gl_mesh->HasSkinning())
    {
        if (clip_index)
        {
            Logger::GetInstance()->info(
                "Skinned mesh '{}' animation settings: play={}, speed={}, "
                "clip_name='{}', clip_index={}.",
                node.GetData().name(),
                proto_scene_mesh.play_animation(),
                speed,
                clip_name,
                *clip_index);
        }
        else
        {
            Logger::GetInstance()->info(
                "Skinned mesh '{}' animation settings: play={}, speed={}, "
                "clip_name='{}'.",
                node.GetData().name(),
                proto_scene_mesh.play_animation(),
                speed,
                clip_name);
        }
    }
}

[[nodiscard]] bool ParseNodeMatrix(
    LevelInterface& level, const proto::NodeMatrix& proto_scene_matrix)
{
    std::unique_ptr<frame::NodeMatrix> scene_matrix = nullptr;
    // Determine if the node should behave as a rotation matrix. Older scene
    // files didn't specify the matrix_type_enum but provided a quaternion when
    // rotation was expected, so infer the rotation flag from either the proto
    // field or the presence of a quaternion.
    bool rotation = proto_scene_matrix.matrix_type_enum() ==
                        proto::NodeMatrix::ROTATION_MATRIX ||
                    proto_scene_matrix.has_quaternion();

    if (proto_scene_matrix.has_matrix())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level),
            ParseUniform(proto_scene_matrix.matrix()),
            rotation);
    }
    else if (proto_scene_matrix.has_quaternion())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level),
            ParseUniform(proto_scene_matrix.quaternion()),
            rotation);
    }
    else
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level), glm::mat4(1.0f), rotation);
    }

    // In case the proto explicitly requested a rotation matrix but we passed
    // "rotation" as false to the constructor (shouldn't happen, but for
    // clarity), force the type here.
    if (rotation)
    {
        scene_matrix->GetData().set_matrix_type_enum(
            proto::NodeMatrix::ROTATION_MATRIX);
    }

    scene_matrix->GetData().set_name(proto_scene_matrix.name());
    scene_matrix->SetParentName(proto_scene_matrix.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_matrix));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseNodeMeshClearBuffer(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    auto node_interface = std::make_unique<frame::NodeMesh>(
        GetFunctor(level), proto_scene_mesh.clean_buffer());
    node_interface->GetData().set_name(proto_scene_mesh.name());
    auto maybe_scene_id = level.AddSceneNode(std::move(node_interface));
    if (!maybe_scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    auto scene_id = maybe_scene_id;
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    node.GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, nullptr);
    level.AddMeshMaterialId(
        scene_id, 0, proto_scene_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeMeshMeshEnum(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    if (proto_scene_mesh.mesh_enum() == proto::NodeMesh::INVALID)
    {
        throw std::runtime_error("Didn't find any mesh file name or any enum.");
    }
    // In this case there is only one material per mesh.
    EntityId mesh_id = 0;
    switch (proto_scene_mesh.mesh_enum())
    {
    case proto::NodeMesh::CUBE: {
        mesh_id = level.GetDefaultMeshCubeId();
        break;
    }
    case proto::NodeMesh::QUAD: {
        mesh_id = level.GetDefaultMeshQuadId();
        break;
    }
    default: {
        throw std::runtime_error(
            std::format(
                "unknown mesh enum value: {}",
                static_cast<int>(proto_scene_mesh.mesh_enum())));
    }
    }
    const EntityId material_id =
        level.GetIdFromName(proto_scene_mesh.material_name());
    ConfigureMaterialProgramsForRenderTime(
        level,
        material_id,
        proto_scene_mesh.render_time_enum());
    auto& mesh = level.GetMeshFromId(mesh_id);
    mesh.GetData().set_render_primitive_enum(
        proto_scene_mesh.render_primitive_enum());
    std::unique_ptr<NodeMesh> node_interface =
        std::make_unique<NodeMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_mesh.name());
    node_interface->SetParentName(proto_scene_mesh.parent());
    node_interface->GetData().set_material_name(
        proto_scene_mesh.material_name());
    node_interface->GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, &mesh);
    level.AddMeshMaterialId(
        scene_id, material_id, proto_scene_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeMeshFileName(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    const EntityId explicit_material_id =
        proto_scene_mesh.material_name().empty()
            ? NullId
            : level.GetIdFromName(proto_scene_mesh.material_name());
    if (!proto_scene_mesh.material_name().empty() && !explicit_material_id)
    {
        Logger::GetInstance()->warn(
            "Material '{}' was not found for mesh '{}'. Falling back to glTF material.",
            proto_scene_mesh.material_name(),
            proto_scene_mesh.name());
    }
    if (explicit_material_id)
    {
        ConfigureMaterialProgramsForRenderTime(
            level,
            explicit_material_id,
            proto_scene_mesh.render_time_enum());
    }
    const auto forced_program_id =
        level.GetRenderPassProgramId(proto_scene_mesh.render_time_enum());
    const auto asset_root = frame::file::FindDirectory("asset");
    auto vec_node_mesh_id = opengl::file::LoadMeshesFromFile(
        level,
        (asset_root / "model" / proto_scene_mesh.file_name()).lexically_normal(),
        proto_scene_mesh.name(),
        "",
        proto_scene_mesh.acceleration_structure_enum(),
        forced_program_id);
    if (vec_node_mesh_id.empty())
    {
        return false;
    }
    int i = 0;
    for (const auto& [node_id, gltf_material_id] : vec_node_mesh_id)
    {
        if (explicit_material_id && gltf_material_id)
        {
            AdoptGltfPbrTextures(
                level, gltf_material_id, explicit_material_id);
        }
        const EntityId material_id =
            explicit_material_id ? explicit_material_id : gltf_material_id;
        ConfigureMaterialProgramsForRenderTime(
            level,
            material_id,
            proto_scene_mesh.render_time_enum());
        auto& node = level.GetSceneNodeFromId(node_id);
        auto& mesh = level.GetMeshFromId(node.GetLocalMesh());
        mesh.GetData().set_file_name(proto_scene_mesh.file_name());
        mesh.GetData().set_render_primitive_enum(
            proto_scene_mesh.render_primitive_enum());
        auto str = std::format("{}.{}", proto_scene_mesh.name(), i);
        mesh.SetName(str);
        auto& mesh_node = dynamic_cast<NodeMesh&>(node);
        mesh_node.GetData().set_file_name(
            proto_scene_mesh.file_name());
        mesh_node.GetData().set_acceleration_structure_enum(
            proto_scene_mesh.acceleration_structure_enum());
        // Rename the node to match the reference name (without the 'Node.'
        // prefix) so serialization will use the same identifier as the input
        // file.
        if (vec_node_mesh_id.size() == 1)
        {
            mesh_node.SetName(proto_scene_mesh.name());
        }
        else
        {
            mesh_node.SetName(str);
        }
        node.SetParentName(proto_scene_mesh.parent());
        if (material_id)
        {
            mesh_node.GetData().set_material_name(
                level.GetNameFromId(material_id));
        }
        else
        {
            mesh_node.GetData().clear_material_name();
        }
        mesh_node.GetData().set_render_time_enum(
            proto_scene_mesh.render_time_enum());
        ApplyAnimationPlayback(proto_scene_mesh, mesh_node, &mesh);
        EnsureRaytracingBvhBuffers(level, material_id, mesh);
        if (!material_id)
        {
            throw std::runtime_error(std::format(
                "No material mapping found for mesh {} in file {}",
                proto_scene_mesh.name(),
                proto_scene_mesh.file_name()));
        }
        level.AddMeshMaterialId(
            node_id,
            material_id,
            proto_scene_mesh.render_time_enum());
        ++i;
    }
    return true;
}

[[nodiscard]] bool ParseNodeMeshStreamInput(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    assert(proto_scene_mesh.has_multi_plugin());
    auto point_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    point_buffer->SetName("point." + proto_scene_mesh.name());
    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    normal_buffer->SetName("normal." + proto_scene_mesh.name());
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto index_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    index_buffer->SetName("index." + proto_scene_mesh.name());
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));
    auto color_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    color_buffer->SetName("color." + proto_scene_mesh.name());
    auto color_buffer_id = level.AddBuffer(std::move(color_buffer));

    // Create a new mesh.
    MeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.color_buffer_id = color_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.render_primitive_enum = proto::NodeMesh::POINT_PRIMITIVE;
    auto mesh = std::make_unique<opengl::Mesh>(level, parameter);
    mesh->SetName("mesh." + proto_scene_mesh.name());
    auto mesh_id = level.AddMesh(std::move(mesh));
    const EntityId material_id =
        level.GetIdFromName(proto_scene_mesh.material_name());
    ConfigureMaterialProgramsForRenderTime(
        level,
        material_id,
        proto_scene_mesh.render_time_enum());
    // Create the node corresponding to the mesh.
    auto& mesh_ref = level.GetMeshFromId(mesh_id);
    mesh_ref.GetData().set_render_primitive_enum(
        proto_scene_mesh.render_primitive_enum());
    std::unique_ptr<NodeMesh> node_interface =
        std::make_unique<NodeMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_mesh.name());
    node_interface->SetParentName(proto_scene_mesh.parent());
    node_interface->GetData().set_material_name(
        proto_scene_mesh.material_name());
    node_interface->GetData().set_acceleration_structure_enum(
        proto_scene_mesh.acceleration_structure_enum());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    auto& node =
        dynamic_cast<NodeMesh&>(level.GetSceneNodeFromId(scene_id));
    node.GetData().set_render_time_enum(
        proto_scene_mesh.render_time_enum());
    ApplyAnimationPlayback(proto_scene_mesh, node, &mesh_ref);
    level.AddMeshMaterialId(
        scene_id, material_id, proto_scene_mesh.render_time_enum());
    if (!scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    return true;
}

[[nodiscard]] bool ParseNodeMesh(
    LevelInterface& level, const proto::NodeMesh& proto_scene_mesh)
{
    // 1st case this is a clean mesh node.
    if (proto_scene_mesh.has_clean_buffer())
    {
        return ParseNodeMeshClearBuffer(level, proto_scene_mesh);
    }
    // 2nd case this is a enum mesh node (CUBE or QUAD).
    if (proto_scene_mesh.has_mesh_enum())
    {
        return ParseNodeMeshMeshEnum(level, proto_scene_mesh);
    }
    // 3rd case this is a mesh file.
    if (proto_scene_mesh.has_file_name())
    {
        return ParseNodeMeshFileName(level, proto_scene_mesh);
    }
    // 4th case stream input.
    if (proto_scene_mesh.has_multi_plugin())
    {
        return ParseNodeMeshStreamInput(level, proto_scene_mesh);
    }
    return false;
}

[[nodiscard]] bool ParseNodeCamera(
    LevelInterface& level, const frame::proto::NodeCamera& proto_scene_camera)
{
    if (proto_scene_camera.fov_degrees() == 0.0)
    {
        throw std::runtime_error("Need field of view degrees in camera.");
    }
    auto scene_camera = std::make_unique<frame::NodeCamera>(
        GetFunctor(level),
        ParseUniform(proto_scene_camera.position()),
        ParseUniform(proto_scene_camera.target()),
        ParseUniform(proto_scene_camera.up()),
        proto_scene_camera.fov_degrees(),
        proto_scene_camera.aspect_ratio(),
        proto_scene_camera.near_clip(),
        proto_scene_camera.far_clip());
    scene_camera->GetData().set_name(proto_scene_camera.name());
    scene_camera->SetParentName(proto_scene_camera.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_camera));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseNodeLight(
    LevelInterface& level, const proto::NodeLight& proto_scene_light)
{
    switch (proto_scene_light.light_type())
    {
    case proto::NodeLight::POINT_LIGHT: {
        EntityId node_id = NullId;
        auto node_light = std::make_unique<frame::NodeLight>(
            GetFunctor(level),
            LightTypeEnum::POINT_LIGHT,
            ParseUniform(proto_scene_light.position()),
            ParseUniform(proto_scene_light.color()));
        node_light->GetData().set_name(proto_scene_light.name());
        node_light->SetParentName(proto_scene_light.parent());
        node_light->GetData().set_shadow_type(proto_scene_light.shadow_type());
        node_id = level.AddSceneNode(std::move(node_light));
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::DIRECTIONAL_LIGHT: {
        EntityId node_id = NullId;
        auto node_light = std::make_unique<frame::NodeLight>(
            GetFunctor(level),
            LightTypeEnum::DIRECTIONAL_LIGHT,
            ParseUniform(proto_scene_light.direction()),
            ParseUniform(proto_scene_light.color()));
        node_light->GetData().set_name(proto_scene_light.name());
        node_light->SetParentName(proto_scene_light.parent());
        node_light->GetData().set_shadow_type(proto_scene_light.shadow_type());
        node_id = level.AddSceneNode(std::move(node_light));
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::AMBIENT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::SPOT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::INVALID_LIGHT:
        [[fallthrough]];
    default:
        throw std::runtime_error(
            std::format(
                "Unknown scene light type {}",
                static_cast<int>(proto_scene_light.light_type())));
    }
    return false;
}

} // End namespace.

[[nodiscard]] bool ParseSceneTreeFile(
    const proto::SceneTree& proto_scene_tree, LevelInterface& level)
{
    level.SetDefaultCameraName(proto_scene_tree.default_camera_name());
    level.SetDefaultRootSceneNodeName(proto_scene_tree.default_root_name());
    for (const auto& proto_matrix : proto_scene_tree.node_matrices())
    {
        if (!ParseNodeMatrix(level, proto_matrix))
        {
            return false;
        }
    }
    for (const auto& proto_mesh : proto_scene_tree.node_meshes())
    {
        if (!ParseNodeMesh(level, proto_mesh))
        {
            return false;
        }
    }
    for (const auto& proto_camera : proto_scene_tree.node_cameras())
    {
        if (!ParseNodeCamera(level, proto_camera))
        {
            return false;
        }
    }
    for (const auto& proto_light : proto_scene_tree.node_lights())
    {
        if (!ParseNodeLight(level, proto_light))
        {
            return false;
        }
    }
    return true;
}

} // End namespace frame::json.





