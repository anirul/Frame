#include "frame/json/serialize_scene_tree.h"
#include "frame/entity_id.h"
#include "frame/json/serialize_uniform.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include <absl/strings/str_split.h>
#include <unordered_set>

namespace frame::json
{

namespace
{

proto::NodeLight::LightTypeEnum SerializeLightType(
    frame::LightTypeEnum light_type)
{
    switch (light_type)
    {
    case frame::LightTypeEnum::AMBIENT_LIGHT:
        return proto::NodeLight::AMBIENT_LIGHT;
    case frame::LightTypeEnum::DIRECTIONAL_LIGHT:
        return proto::NodeLight::DIRECTIONAL_LIGHT;
    case frame::LightTypeEnum::INVALID_LIGHT:
        return proto::NodeLight::INVALID_LIGHT;
    case frame::LightTypeEnum::POINT_LIGHT:
        return proto::NodeLight::POINT_LIGHT;
    case frame::LightTypeEnum::SPOT_LIGHT:
        return proto::NodeLight::SPOT_LIGHT;
    }
    throw std::runtime_error("Unknown error???");
}

proto::NodeCamera SerializeNodeCamera(const NodeInterface& node_interface)
{
    proto::NodeCamera proto_scene_camera;
    const NodeCamera& node_camera =
        dynamic_cast<const NodeCamera&>(node_interface);
    proto_scene_camera.set_name(node_camera.GetData().name());
    proto_scene_camera.set_parent(node_camera.GetParentName());
    const Camera& camera = node_camera.GetCamera();
    proto::UniformVector3 position =
        SerializeUniformVector3(camera.GetPosition());
    *proto_scene_camera.mutable_position() = position;
    proto::UniformVector3 target = SerializeUniformVector3(camera.GetFront());
    *proto_scene_camera.mutable_target() = target;
    proto::UniformVector3 up = SerializeUniformVector3(camera.GetUp());
    *proto_scene_camera.mutable_up() = up;
    proto_scene_camera.set_fov_degrees(camera.GetFovDegrees());
    proto_scene_camera.set_aspect_ratio(camera.GetAspectRatio());
    proto_scene_camera.set_near_clip(camera.GetNearClip());
    proto_scene_camera.set_far_clip(camera.GetFarClip());
    return proto_scene_camera;
}

proto::NodeLight SerializeNodeLight(const NodeInterface& node_interface)
{
    proto::NodeLight proto_scene_light;
    const NodeLight& node_light =
        dynamic_cast<const NodeLight&>(node_interface);
    proto_scene_light.set_name(node_light.GetData().name());
    proto_scene_light.set_parent(node_light.GetParentName());
    proto_scene_light.set_light_type(node_light.GetData().light_type());
    *proto_scene_light.mutable_position() = node_light.GetData().position();
    *proto_scene_light.mutable_direction() = node_light.GetData().direction();
    proto_scene_light.set_dot_inner_limit(
        node_light.GetData().dot_inner_limit());
    proto_scene_light.set_dot_outer_limit(
        node_light.GetData().dot_outer_limit());
    *proto_scene_light.mutable_color() = node_light.GetData().color();
    return proto_scene_light;
}

proto::NodeMatrix SerializeNodeMatrix(const NodeInterface& node_interface)
{
    proto::NodeMatrix proto_scene_matrix;
    const NodeMatrix& node_matrix =
        dynamic_cast<const NodeMatrix&>(node_interface);
    proto_scene_matrix.set_name(node_matrix.GetData().name());
    proto_scene_matrix.set_parent(node_matrix.GetParentName());
    proto_scene_matrix.set_matrix_type_enum(
        node_matrix.GetData().matrix_type_enum());
    if (node_matrix.GetData().matrix_type_enum() ==
            proto::NodeMatrix::ROTATION_MATRIX &&
        node_matrix.GetData().has_quaternion())
    {
        *proto_scene_matrix.mutable_quaternion() =
            node_matrix.GetData().quaternion();
    }
    else
    {
        *proto_scene_matrix.mutable_matrix() = node_matrix.GetData().matrix();
    }
    return proto_scene_matrix;
}

void SerializeNodeMeshEnum(
    proto::NodeMesh& proto_node_mesh,
    const NodeMesh& node_mesh,
    const LevelInterface& level_interface)
{
    proto_node_mesh.set_mesh_enum(proto::NodeMesh::INVALID);
    if (node_mesh.GetLocalMesh() ==
        level_interface.GetDefaultMeshCubeId())
    {
        proto_node_mesh.set_mesh_enum(proto::NodeMesh::CUBE);
    }
    if (node_mesh.GetLocalMesh() ==
        level_interface.GetDefaultMeshQuadId())
    {
        proto_node_mesh.set_mesh_enum(proto::NodeMesh::QUAD);
    }
    if (proto_node_mesh.mesh_enum() == proto::NodeMesh::INVALID)
    {
        throw std::runtime_error(
            std::format(
                "Couldn't find any mesh for this node: [{}].",
                node_mesh.GetData().name()));
    }
    proto_node_mesh.set_material_name(
        node_mesh.GetData().material_name());
    proto_node_mesh.set_render_time_enum(
        node_mesh.GetData().render_time_enum());
}

void SerializeNodeMeshFileName(
    proto::NodeMesh& proto_node_mesh,
    const std::string& mesh_name,
    const NodeMesh& node_mesh,
    const LevelInterface& level_interface)
{
    proto_node_mesh.set_file_name(mesh_name);
    proto_node_mesh.set_material_name(
        node_mesh.GetData().material_name());
    proto_node_mesh.set_render_time_enum(
        node_mesh.GetData().render_time_enum());
}

proto::CleanBuffer SerializeCleanBuffer(std::uint32_t clean_buffer)
{
    proto::CleanBuffer proto_clean_buffer;
    if (clean_buffer & proto::CleanBuffer::CLEAR_COLOR)
    {
        proto_clean_buffer.add_values(proto::CleanBuffer::CLEAR_COLOR);
    }
    if (clean_buffer & proto::CleanBuffer::CLEAR_DEPTH)
    {
        proto_clean_buffer.add_values(proto::CleanBuffer::CLEAR_DEPTH);
    }
    return proto_clean_buffer;
}

proto::NodeMesh SerializeNodeMesh(
    const NodeInterface& node_interface, const LevelInterface& level_interface)
{
    proto::NodeMesh proto_node_mesh;
    const NodeMesh& node_mesh =
        dynamic_cast<const NodeMesh&>(node_interface);
    proto_node_mesh.set_name(node_mesh.GetData().name());
    proto_node_mesh.set_parent(node_mesh.GetParentName());

    // Determine which representation of the mesh we should serialize.
    if (node_mesh.GetLocalMesh() == NullId)
    {
        *proto_node_mesh.mutable_clean_buffer() =
            node_mesh.GetData().clean_buffer();
    }
    else if (
        node_mesh.GetLocalMesh() ==
            level_interface.GetDefaultMeshCubeId() ||
        node_mesh.GetLocalMesh() ==
            level_interface.GetDefaultMeshQuadId())
    {
        SerializeNodeMeshEnum(
            proto_node_mesh, node_mesh, level_interface);
    }
    else
    {
        std::string mesh_name;
        if (node_mesh.GetData().has_file_name())
        {
            mesh_name = node_mesh.GetData().file_name();
        }
        else
        {
            mesh_name =
                level_interface
                    .GetMeshFromId(node_mesh.GetLocalMesh())
                    .GetData()
                    .file_name();
        }
        SerializeNodeMeshFileName(
            proto_node_mesh,
            mesh_name,
            node_mesh,
            level_interface);
    }

    proto_node_mesh.set_material_name(
        node_mesh.GetData().material_name());
    proto_node_mesh.set_render_time_enum(
        node_mesh.GetData().render_time_enum());
    proto_node_mesh.set_acceleration_structure_enum(
        node_mesh.GetData().acceleration_structure_enum());
    proto_node_mesh.set_play_animation(
        node_mesh.GetData().play_animation());
    if (node_mesh.GetData().has_animation_speed())
    {
        proto_node_mesh.set_animation_speed(
            node_mesh.GetData().animation_speed());
    }
    if (node_mesh.GetData().has_animation_clip_name())
    {
        proto_node_mesh.set_animation_clip_name(
            node_mesh.GetData().animation_clip_name());
    }
    if (node_mesh.GetData().has_animation_clip_index())
    {
        proto_node_mesh.set_animation_clip_index(
            node_mesh.GetData().animation_clip_index());
    }
    return proto_node_mesh;
}

void SerializeNode(
    proto::SceneTree& proto_scene_tree,
    EntityId id,
    const LevelInterface& level_interface,
    std::unordered_set<EntityId>& visited)
{
    if (visited.contains(id))
        return;
    visited.insert(id);

    NodeInterface& node_interface = level_interface.GetSceneNodeFromId(id);
    switch (node_interface.GetNodeType())
    {
    case NodeTypeEnum::NODE_CAMERA:
        *proto_scene_tree.add_node_cameras() =
            SerializeNodeCamera(node_interface);
        break;
    case NodeTypeEnum::NODE_LIGHT:
        *proto_scene_tree.add_node_lights() =
            SerializeNodeLight(node_interface);
        break;
    case NodeTypeEnum::NODE_MATRIX:
        *proto_scene_tree.add_node_matrices() =
            SerializeNodeMatrix(node_interface);
        break;
    case NodeTypeEnum::NODE_MESH:
        *proto_scene_tree.add_node_meshes() =
            SerializeNodeMesh(node_interface, level_interface);
        break;
    case NodeTypeEnum::NODE_UKNOWN:
        [[fallthrough]];
    default:
        throw std::runtime_error(
            std::format(
                "Unknown node type [{}]?",
                static_cast<int>(node_interface.GetNodeType())));
    }
    for (const auto child_id : level_interface.GetChildList(id))
    {
        SerializeNode(proto_scene_tree, child_id, level_interface, visited);
    }
}

} // End anonymous namespace.

proto::SceneTree SerializeSceneTree(const LevelInterface& level_interface)
{
    proto::SceneTree proto_scene_tree;
    proto_scene_tree.set_default_camera_name(
        level_interface.GetNameFromId(level_interface.GetDefaultCameraId()));
    proto_scene_tree.set_default_root_name(level_interface.GetNameFromId(
        level_interface.GetDefaultRootSceneNodeId()));
    std::unordered_set<EntityId> visited;
    SerializeNode(
        proto_scene_tree,
        level_interface.GetDefaultRootSceneNodeId(),
        level_interface,
        visited);
    // Serialize nodes that do not have a parent (independent roots).
    for (const auto node_id : level_interface.GetSceneNodes())
    {
        if (node_id == level_interface.GetDefaultRootSceneNodeId())
            continue;
        if (level_interface.GetParentId(node_id) == NullId)
        {
            SerializeNode(proto_scene_tree, node_id, level_interface, visited);
        }
    }
    return proto_scene_tree;
}

} // End namespace frame::json.


