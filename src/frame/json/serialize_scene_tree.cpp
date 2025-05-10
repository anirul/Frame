#include "frame/json/serialize_scene_tree.h"
#include "frame/json/serialize_uniform.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"

namespace frame::json
{

proto::SceneLight::LightTypeEnum SerializeLightType(
    frame::LightTypeEnum light_type)
{
    switch (light_type)
    {
    case frame::LightTypeEnum::AMBIENT_LIGHT:
        return proto::SceneLight::AMBIENT_LIGHT;
    case frame::LightTypeEnum::DIRECTIONAL_LIGHT:
        return proto::SceneLight::DIRECTIONAL_LIGHT;
    case frame::LightTypeEnum::INVALID_LIGHT:
        return proto::SceneLight::INVALID_LIGHT;
    case frame::LightTypeEnum::POINT_LIGHT:
        return proto::SceneLight::POINT_LIGHT;
    case frame::LightTypeEnum::SPOT_LIGHT:
        return proto::SceneLight::SPOT_LIGHT;
    }
    throw std::runtime_error("Unknown error???");
}

proto::SceneCamera SerializeNodeCamera(const NodeInterface& node_interface)
{
    proto::SceneCamera proto_scene_camera;
    const NodeCamera& node_camera =
        dynamic_cast<const NodeCamera&>(node_interface);
    proto_scene_camera.set_name(node_camera.GetName());
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

proto::SceneLight SerializeNodeLight(const NodeInterface& node_interface)
{
    proto::SceneLight proto_scene_light;
    const NodeLight& node_light =
        dynamic_cast<const NodeLight&>(node_interface);
    proto_scene_light.set_name(node_light.GetName());
    proto_scene_light.set_parent(node_light.GetParentName());
    proto_scene_light.set_light_type(SerializeLightType(node_light.GetType()));
    *proto_scene_light.mutable_position() =
        SerializeUniformVector3(node_light.GetPosition());
    *proto_scene_light.mutable_direction() =
        SerializeUniformVector3(node_light.GetDirection());
    proto_scene_light.set_dot_inner_limit(node_light.GetDotInner());
    proto_scene_light.set_dot_outer_limit(node_light.GetDotOuter());
    *proto_scene_light.mutable_color() =
        SerializeUniformVector3(node_light.GetColor());
    return proto_scene_light;
}

proto::SceneMatrix SerializeNodeMatrix(const NodeInterface& node_interface)
{
    proto::SceneMatrix proto_scene_matrix;
    const NodeMatrix& node_matrix =
        dynamic_cast<const NodeMatrix&>(node_interface);
    proto_scene_matrix.set_name(node_matrix.GetName());
    proto_scene_matrix.set_parent(node_matrix.GetParentName());
    if (node_matrix.IsRotationEnabled())
    {
        glm::quat glm_quat = node_matrix.GetQuaternion();
        *proto_scene_matrix.mutable_quaternion() = SerializeUniformVector4(
            glm::vec4(glm_quat.x, glm_quat.y, glm_quat.z, glm_quat.w));
    }
    else
    {
        *proto_scene_matrix.mutable_matrix() =
            SerializeUniformMatrix4(node_matrix.GetMatrix());
    }
    return proto_scene_matrix;
}

proto::SceneStaticMesh SerializeNodeStaticMesh(
    const NodeInterface& node_interface, const LevelInterface& level_interface)
{
    proto::SceneStaticMesh proto_scene_static_mesh;
    const NodeStaticMesh& node_static_mesh =
        dynamic_cast<const NodeStaticMesh&>(node_interface);
    proto_scene_static_mesh.set_name(node_static_mesh.GetName());
    proto_scene_static_mesh.set_parent(node_static_mesh.GetParentName());

    return proto_scene_static_mesh;
}

void SerializeNode(
    proto::SceneTree& proto_scene_tree,
    EntityId id,
    const LevelInterface& level_interface)
{
    NodeInterface& node_interface = level_interface.GetSceneNodeFromId(id);
    switch (node_interface.GetNodeType())
    {
    case NodeTypeEnum::NODE_CAMERA:
        *proto_scene_tree.add_scene_cameras() =
            SerializeNodeCamera(node_interface);
        break;
    case NodeTypeEnum::NODE_LIGHT:
        *proto_scene_tree.add_scene_lights() =
            SerializeNodeLight(node_interface);
        break;
    case NodeTypeEnum::NODE_MATRIX:
        *proto_scene_tree.add_scene_matrices() =
            SerializeNodeMatrix(node_interface);
        break;
    case NodeTypeEnum::NODE_STATIC_MESH:
        *proto_scene_tree.add_scene_static_meshes() =
            SerializeNodeStaticMesh(node_interface, level_interface);
        break;
    case NodeTypeEnum::NODE_UKNOWN:
        [[fallthrough]];
    default:
        throw std::runtime_error(
            std::format(
                "Unknown node type [{}]?",
                static_cast<int>(node_interface.GetNodeType())));
    }
    for (const auto id : level_interface.GetChildList(id))
    {
        SerializeNode(proto_scene_tree, id, level_interface);
    }
}

proto::SceneTree SerializeSceneTree(const LevelInterface& level_interface)
{
    proto::SceneTree proto_scene_tree;
    auto maybe_camera_name =
        level_interface.GetNameFromId(level_interface.GetDefaultCameraId());
    if (!maybe_camera_name)
    {
        throw std::runtime_error(
            "Couldn't get the name of the default camera?");
    }
    proto_scene_tree.set_default_camera_name(maybe_camera_name.value());
    auto maybe_root_name = level_interface.GetNameFromId(
        level_interface.GetDefaultRootSceneNodeId());
    if (!maybe_root_name)
    {
        throw std::runtime_error("Couldn't get the default root name?");
    }
    proto_scene_tree.set_default_root_name(maybe_root_name.value());
    SerializeNode(
        proto_scene_tree,
        level_interface.GetDefaultRootSceneNodeId(),
        level_interface);
    return proto_scene_tree;
}

} // End namespace frame::json.
