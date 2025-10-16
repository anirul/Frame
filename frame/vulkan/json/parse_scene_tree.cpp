#include "frame/vulkan/json/parse_scene_tree.h"

#include <format>
#include <stdexcept>

#include "frame/json/parse_uniform.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan::json
{

namespace
{

std::function<NodeInterface*(const std::string&)> MakeResolver(
    LevelInterface& level)
{
    return [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id)
        {
            throw std::runtime_error(std::format(
                "Unable to resolve scene node named {}", name));
        }
        return &level.GetSceneNodeFromId(maybe_id);
    };
}

bool ParseNodeMatrix(
    LevelInterface& level,
    const frame::proto::NodeMatrix& proto_matrix)
{
    std::unique_ptr<frame::NodeMatrix> node_matrix;
    const bool rotation =
        proto_matrix.matrix_type_enum() ==
            frame::proto::NodeMatrix::ROTATION_MATRIX ||
        proto_matrix.has_quaternion();

    if (proto_matrix.has_matrix())
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level),
            frame::json::ParseUniform(proto_matrix.matrix()),
            rotation);
    }
    else if (proto_matrix.has_quaternion())
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level),
            frame::json::ParseUniform(proto_matrix.quaternion()),
            rotation);
    }
    else
    {
        node_matrix = std::make_unique<frame::NodeMatrix>(
            MakeResolver(level), glm::mat4(1.0f), rotation);
    }

    if (rotation)
    {
        node_matrix->GetData().set_matrix_type_enum(
            frame::proto::NodeMatrix::ROTATION_MATRIX);
    }

    node_matrix->GetData().set_name(proto_matrix.name());
    node_matrix->SetParentName(proto_matrix.parent());
    auto maybe_id = level.AddSceneNode(std::move(node_matrix));
    return static_cast<bool>(maybe_id);
}

bool ParseNodeStaticMeshFromEnum(
    LevelInterface& level,
    const frame::proto::NodeStaticMesh& proto_mesh)
{
    if (proto_mesh.mesh_enum() == frame::proto::NodeStaticMesh::INVALID)
    {
        throw std::runtime_error("Static mesh enum is invalid.");
    }

    EntityId mesh_id = frame::NullId;
    switch (proto_mesh.mesh_enum())
    {
    case frame::proto::NodeStaticMesh::CUBE:
        mesh_id = level.GetDefaultStaticMeshCubeId();
        break;
    case frame::proto::NodeStaticMesh::QUAD:
        mesh_id = level.GetDefaultStaticMeshQuadId();
        break;
    default:
        throw std::runtime_error("Unsupported static mesh enum for Vulkan.");
    }

    if (!mesh_id)
    {
        throw std::runtime_error("Static mesh not available in level.");
    }

    auto material_id = level.GetIdFromName(proto_mesh.material_name());
    if (!material_id)
    {
        throw std::runtime_error(std::format(
            "Material {} not found for static mesh {}.",
            proto_mesh.material_name(),
            proto_mesh.name()));
    }

    auto node = std::make_unique<frame::NodeStaticMesh>(
        MakeResolver(level), mesh_id);
    node->GetData().set_name(proto_mesh.name());
    node->SetParentName(proto_mesh.parent());
    node->GetData().set_material_name(proto_mesh.material_name());
    node->GetData().set_render_time_enum(proto_mesh.render_time_enum());

    auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(scene_id, material_id, proto_mesh.render_time_enum());
    return true;
}

bool ParseNodeStaticMeshCleanBuffer(
    LevelInterface& level,
    const frame::proto::NodeStaticMesh& proto_mesh)
{
    auto node = std::make_unique<frame::NodeStaticMesh>(
        MakeResolver(level), proto_mesh.clean_buffer());
    node->GetData().set_name(proto_mesh.name());
    node->SetParentName(proto_mesh.parent());
    node->GetData().set_render_time_enum(proto_mesh.render_time_enum());
    auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(
        scene_id, frame::NullId, proto_mesh.render_time_enum());
    return true;
}

bool ParseNodeStaticMesh(
    LevelInterface& level,
    const frame::proto::NodeStaticMesh& proto_mesh)
{
    if (proto_mesh.has_clean_buffer())
    {
        return ParseNodeStaticMeshCleanBuffer(level, proto_mesh);
    }
    if (proto_mesh.has_mesh_enum())
    {
        return ParseNodeStaticMeshFromEnum(level, proto_mesh);
    }
    if (proto_mesh.has_file_name())
    {
        throw std::runtime_error(
            "Loading static meshes from file is not implemented for Vulkan yet.");
    }
    if (proto_mesh.has_multi_plugin())
    {
        throw std::runtime_error("Streamed static meshes are not implemented for Vulkan yet.");
    }
    return false;
}

bool ParseNodeCamera(
    LevelInterface& level,
    const frame::proto::NodeCamera& proto_camera)
{
    if (proto_camera.fov_degrees() == 0.0)
    {
        throw std::runtime_error("Camera must define a field of view.");
    }
    auto camera = std::make_unique<frame::NodeCamera>(
        MakeResolver(level),
        frame::json::ParseUniform(proto_camera.position()),
        frame::json::ParseUniform(proto_camera.target()),
        frame::json::ParseUniform(proto_camera.up()),
        proto_camera.fov_degrees(),
        proto_camera.aspect_ratio(),
        proto_camera.near_clip(),
        proto_camera.far_clip());
    camera->GetData().set_name(proto_camera.name());
    camera->SetParentName(proto_camera.parent());
    return static_cast<bool>(level.AddSceneNode(std::move(camera)));
}

bool ParseNodeLight(
    LevelInterface& level,
    const frame::proto::NodeLight& proto_light)
{
    switch (proto_light.light_type())
    {
    case frame::proto::NodeLight::POINT_LIGHT: {
        auto light = std::make_unique<frame::NodeLight>(
            MakeResolver(level),
            frame::LightTypeEnum::POINT_LIGHT,
            frame::json::ParseUniform(proto_light.position()),
            frame::json::ParseUniform(proto_light.color()));
        light->GetData().set_name(proto_light.name());
        light->SetParentName(proto_light.parent());
        light->GetData().set_shadow_type(proto_light.shadow_type());
        return static_cast<bool>(level.AddSceneNode(std::move(light)));
    }
    case frame::proto::NodeLight::DIRECTIONAL_LIGHT: {
        auto light = std::make_unique<frame::NodeLight>(
            MakeResolver(level),
            frame::LightTypeEnum::DIRECTIONAL_LIGHT,
            frame::json::ParseUniform(proto_light.direction()),
            frame::json::ParseUniform(proto_light.color()));
        light->GetData().set_name(proto_light.name());
        light->SetParentName(proto_light.parent());
        light->GetData().set_shadow_type(proto_light.shadow_type());
        return static_cast<bool>(level.AddSceneNode(std::move(light)));
    }
    default:
        throw std::runtime_error("Unsupported light type for Vulkan parser.");
    }
}

} // namespace

bool ParseSceneTree(
    const frame::proto::SceneTree& proto_scene_tree,
    LevelInterface& level)
{
    level.SetDefaultCameraName(proto_scene_tree.default_camera_name());
    level.SetDefaultRootSceneNodeName(proto_scene_tree.default_root_name());

    for (const auto& node_matrix : proto_scene_tree.node_matrices())
    {
        if (!ParseNodeMatrix(level, node_matrix))
        {
            return false;
        }
    }

    for (const auto& node_static_mesh : proto_scene_tree.node_static_meshes())
    {
        if (!ParseNodeStaticMesh(level, node_static_mesh))
        {
            return false;
        }
    }

    for (const auto& node_camera : proto_scene_tree.node_cameras())
    {
        if (!ParseNodeCamera(level, node_camera))
        {
            return false;
        }
    }

    for (const auto& node_light : proto_scene_tree.node_lights())
    {
        if (!ParseNodeLight(level, node_light))
        {
            return false;
        }
    }

    return true;
}

} // namespace frame::vulkan::json
