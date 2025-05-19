#include "frame/json/parse_scene_tree.h"

#include <fmt/core.h>

#include "frame/file/file_system.h"
#include "frame/file/obj.h"
#include "frame/json/parse_uniform.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/file/load_static_mesh.h"
#include "frame/opengl/static_mesh.h"

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
            throw std::runtime_error(fmt::format("No id from name: {}", name));
        }
        EntityId id = maybe_id;
        return &level.GetSceneNodeFromId(id);
    };
}

[[nodiscard]] bool ParseNodeMatrix(
    LevelInterface& level, const proto::NodeMatrix& proto_scene_matrix)
{
    std::unique_ptr<frame::NodeMatrix> scene_matrix = nullptr;
    if (proto_scene_matrix.has_matrix())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level), ParseUniform(proto_scene_matrix.matrix()));
    }
    else if (proto_scene_matrix.has_quaternion())
    {
        scene_matrix = std::make_unique<frame::NodeMatrix>(
            GetFunctor(level), ParseUniform(proto_scene_matrix.quaternion()));
    }
    else
    {
        scene_matrix =
            std::make_unique<frame::NodeMatrix>(GetFunctor(level), glm::mat4(1.0f));
    }
    scene_matrix->GetData().set_name(proto_scene_matrix.name());
    scene_matrix->SetParentName(proto_scene_matrix.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_matrix));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseNodeStaticMeshClearBuffer(
    LevelInterface& level,
    const proto::NodeStaticMesh& proto_scene_static_mesh)
{
    auto node_interface = std::make_unique<frame::NodeStaticMesh>(
        GetFunctor(level), proto_scene_static_mesh.clean_buffer());
    node_interface->GetData().set_name(proto_scene_static_mesh.name());
    auto maybe_scene_id = level.AddSceneNode(std::move(node_interface));
    if (!maybe_scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    level.AddMeshMaterialId(
        maybe_scene_id, 0, proto_scene_static_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeStaticMeshMeshEnum(
    LevelInterface& level,
    const proto::NodeStaticMesh& proto_scene_static_mesh)
{
    if (proto_scene_static_mesh.mesh_enum() == proto::NodeStaticMesh::INVALID)
    {
        throw std::runtime_error(
            "Didn't find any mesh file name or any enum.");
    }
    // In this case there is only one material per mesh.
    EntityId mesh_id = 0;
    switch (proto_scene_static_mesh.mesh_enum())
    {
    case proto::NodeStaticMesh::CUBE: {
        mesh_id = level.GetDefaultStaticMeshCubeId();
        break;
    }
    case proto::NodeStaticMesh::QUAD: {
        mesh_id = level.GetDefaultStaticMeshQuadId();
        break;
    }
    default: {
        throw std::runtime_error(fmt::format(
            "unknown mesh enum value: {}",
            static_cast<int>(proto_scene_static_mesh.mesh_enum())));
    }
    }
    const EntityId material_id = level.GetIdFromName(proto_scene_static_mesh.material_name());
    auto& mesh = level.GetStaticMeshFromId(mesh_id);
    mesh.GetData().set_render_primitive_enum(
        proto_scene_static_mesh.render_primitive_enum());
    std::unique_ptr<NodeStaticMesh> node_interface =
        std::make_unique<NodeStaticMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_static_mesh.name());
    node_interface->SetParentName(proto_scene_static_mesh.parent());
    node_interface->GetData().set_material_name(proto_scene_static_mesh.material_name());
    level.AddMeshMaterialId(
        level.AddSceneNode(std::move(node_interface)),
        material_id,
        proto_scene_static_mesh.render_time_enum());
    return true;
}

[[nodiscard]] bool ParseNodeStaticMeshFileName(
    LevelInterface& level,
    const proto::NodeStaticMesh& proto_scene_static_mesh)
{
    auto vec_node_mesh_id = opengl::file::LoadStaticMeshesFromFile(
        level,
        "asset/model/" + proto_scene_static_mesh.file_name(),
        proto_scene_static_mesh.name(),
        proto_scene_static_mesh.material_name());
    if (vec_node_mesh_id.empty())
    {
        return false;
    }
    int i = 0;
    for (const auto node_mesh_id : vec_node_mesh_id)
    {
        auto& node = level.GetSceneNodeFromId(node_mesh_id);
        auto& mesh = level.GetStaticMeshFromId(node.GetLocalMesh());
        mesh.GetData().set_file_name(proto_scene_static_mesh.file_name());
        mesh.GetData().set_render_primitive_enum(
            proto_scene_static_mesh.render_primitive_enum());
        auto str = fmt::format("{}.{}", proto_scene_static_mesh.name(), i);
        mesh.SetName(str);
        node.SetParentName(proto_scene_static_mesh.parent());
        ++i;
    }
    return true;
}

[[nodiscard]] bool ParseNodeStaticMeshStreamInput(
    LevelInterface& level,
    const proto::NodeStaticMesh& proto_scene_static_mesh)
{
    assert(proto_scene_static_mesh.has_multi_plugin());
    auto point_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    point_buffer->SetName("point." + proto_scene_static_mesh.name());
    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    normal_buffer->SetName("normal." + proto_scene_static_mesh.name());
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto index_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    index_buffer->SetName("index." + proto_scene_static_mesh.name());
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));
    auto color_buffer = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ARRAY_BUFFER,
        opengl::BufferUsageEnum::STREAM_DRAW);
    color_buffer->SetName("color." + proto_scene_static_mesh.name());
    auto color_buffer_id = level.AddBuffer(std::move(color_buffer));

    // Create a new static mesh.
    StaticMeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.color_buffer_id = color_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.render_primitive_enum = proto::NodeStaticMesh::POINT_PRIMITIVE;
    auto mesh = std::make_unique<opengl::StaticMesh>(level, parameter);
    mesh->SetName("mesh." + proto_scene_static_mesh.name());
    auto mesh_id = level.AddStaticMesh(std::move(mesh));
    const EntityId material_id =
        level.GetIdFromName(proto_scene_static_mesh.material_name());
    // Create the node corresponding to the mesh.
    auto& mesh_ref = level.GetStaticMeshFromId(mesh_id);
    mesh_ref.GetData().set_render_primitive_enum(
        proto_scene_static_mesh.render_primitive_enum());
    std::unique_ptr<NodeStaticMesh> node_interface =
        std::make_unique<NodeStaticMesh>(GetFunctor(level), mesh_id);
    node_interface->GetData().set_name(proto_scene_static_mesh.name());
    node_interface->SetParentName(proto_scene_static_mesh.parent());
    node_interface->GetData().set_material_name(proto_scene_static_mesh.material_name());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    level.AddMeshMaterialId(
        scene_id, material_id, proto_scene_static_mesh.render_time_enum());
    if (!scene_id)
    {
        throw std::runtime_error("No scene Id.");
    }
    return true;
}

[[nodiscard]] bool ParseNodeStaticMesh(
    LevelInterface& level,
    const proto::NodeStaticMesh& proto_scene_static_mesh)
{
    // 1st case this is a clean static mesh node.
    if (proto_scene_static_mesh.has_clean_buffer())
    {
        return ParseNodeStaticMeshClearBuffer(level, proto_scene_static_mesh);
    }
    // 2nd case this is a enum static mesh node (CUBE or QUAD).
    if (proto_scene_static_mesh.has_mesh_enum())
    {
        return ParseNodeStaticMeshMeshEnum(level, proto_scene_static_mesh);
    }
    // 3rd case this is a mesh file.
    if (proto_scene_static_mesh.has_file_name())
    {
        return ParseNodeStaticMeshFileName(level, proto_scene_static_mesh);
    }
    // 4th case stream input.
    if (proto_scene_static_mesh.has_multi_plugin())
    {
        return ParseNodeStaticMeshStreamInput(level, proto_scene_static_mesh);
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
        if (proto_scene_light.shadow_type() == proto::NodeLight::NO_SHADOW)
        {
            std::unique_ptr<NodeInterface> node_light =
                std::make_unique<frame::NodeLight>(
                    GetFunctor(level),
                    LightTypeEnum::POINT_LIGHT,
                    ParseUniform(proto_scene_light.position()),
                    ParseUniform(proto_scene_light.color()));
            node_id = level.AddSceneNode(std::move(node_light));
        }
        else
        {
            std::unique_ptr<NodeInterface> node_light =
                std::make_unique<frame::NodeLight>(
                    GetFunctor(level),
                    LightTypeEnum::POINT_LIGHT,
                    static_cast<ShadowTypeEnum>(
                        proto_scene_light.shadow_type()),
                    proto_scene_light.shadow_texture(),
                    ParseUniform(proto_scene_light.position()),
                    ParseUniform(proto_scene_light.color()));
            node_id = level.AddSceneNode(std::move(node_light));
        }
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::DIRECTIONAL_LIGHT: {
        EntityId node_id = NullId;
        if (proto_scene_light.shadow_type() == proto::NodeLight::NO_SHADOW)
        {
            std::unique_ptr<NodeInterface> node_light =
                std::make_unique<frame::NodeLight>(
                    GetFunctor(level),
                    LightTypeEnum::DIRECTIONAL_LIGHT,
                    ParseUniform(proto_scene_light.direction()),
                    ParseUniform(proto_scene_light.color()));
            node_id = level.AddSceneNode(std::move(node_light));
        }
        else
        {
            std::unique_ptr<NodeInterface> node_light =
                std::make_unique<frame::NodeLight>(
                    GetFunctor(level),
                    LightTypeEnum::DIRECTIONAL_LIGHT,
                    static_cast<ShadowTypeEnum>(
                        proto_scene_light.shadow_type()),
                    proto_scene_light.shadow_texture(),
                    ParseUniform(proto_scene_light.direction()),
                    ParseUniform(proto_scene_light.color()));
            node_id = level.AddSceneNode(std::move(node_light));
        }
        return static_cast<bool>(node_id);
    }
    case proto::NodeLight::AMBIENT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::SPOT_LIGHT:
        [[fallthrough]];
    case proto::NodeLight::INVALID_LIGHT:
        [[fallthrough]];
    default:
        throw std::runtime_error(fmt::format(
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
    for (const auto& proto_matrix : proto_scene_tree.scene_matrices())
    {
        if (!ParseNodeMatrix(level, proto_matrix))
        {
            return false;
        }
    }
    for (const auto& proto_static_mesh : proto_scene_tree.scene_static_meshes())
    {
        if (!ParseNodeStaticMesh(level, proto_static_mesh))
        {
            return false;
        }
    }
    for (const auto& proto_camera : proto_scene_tree.scene_cameras())
    {
        if (!ParseNodeCamera(level, proto_camera))
        {
            return false;
        }
    }
    for (const auto& proto_light : proto_scene_tree.scene_lights())
    {
        if (!ParseNodeLight(level, proto_light))
        {
            return false;
        }
    }
    return true;
}

} // End namespace frame::json.
