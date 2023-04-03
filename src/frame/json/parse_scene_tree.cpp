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

namespace frame::proto {

namespace {

std::function<NodeInterface*(const std::string& name)> GetFunctor(LevelInterface& level) {
    return [&level](const std::string& name) -> NodeInterface* {
        auto maybe_id = level.GetIdFromName(name);
        if (!maybe_id) {
            throw std::runtime_error(fmt::format("No id from name: {}", name));
        }
        EntityId id = maybe_id;
        return &level.GetSceneNodeFromId(id);
    };
}

[[nodiscard]] bool ParseSceneMatrix(LevelInterface& level, const SceneMatrix& proto_scene_matrix) {
    std::unique_ptr<NodeMatrix> scene_matrix = nullptr;
    if (proto_scene_matrix.has_matrix()) {
        scene_matrix = std::make_unique<NodeMatrix>(GetFunctor(level),
                                                    ParseUniform(proto_scene_matrix.matrix()));
    } else if (proto_scene_matrix.has_quaternion()) {
        scene_matrix = std::make_unique<NodeMatrix>(GetFunctor(level),
                                                    ParseUniform(proto_scene_matrix.quaternion()));
    } else {
        scene_matrix = std::make_unique<NodeMatrix>(GetFunctor(level), glm::mat4(1.0f));
    }
    scene_matrix->SetName(proto_scene_matrix.name());
    scene_matrix->SetParentName(proto_scene_matrix.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_matrix));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseSceneStaticMeshClearBuffer(LevelInterface& level,
                                                   const SceneStaticMesh& proto_scene_static_mesh) {
    auto node_interface =
        std::make_unique<NodeStaticMesh>(GetFunctor(level), proto_scene_static_mesh.clean_buffer());
    node_interface->SetName(proto_scene_static_mesh.name());
    auto maybe_scene_id = level.AddSceneNode(std::move(node_interface));
    if (!maybe_scene_id) throw std::runtime_error("No scene Id.");
    level.AddMeshMaterialId(maybe_scene_id, 0);
    return true;
}

[[nodiscard]] bool ParseSceneStaticMeshMeshEnum(LevelInterface& level,
                                                const SceneStaticMesh& proto_scene_static_mesh) {
    if (proto_scene_static_mesh.mesh_enum() == SceneStaticMesh::INVALID) {
        throw std::runtime_error("Didn't find any mesh file name or any enum.");
    }
    // In this case there is only one material per mesh.
    EntityId mesh_id = 0;
    switch (proto_scene_static_mesh.mesh_enum()) {
        case SceneStaticMesh::CUBE: {
            auto maybe_mesh_id = level.GetDefaultStaticMeshCubeId();
            if (!maybe_mesh_id) return false;
            mesh_id = maybe_mesh_id;
            break;
        }
        case SceneStaticMesh::QUAD: {
            auto maybe_mesh_id = level.GetDefaultStaticMeshQuadId();
            if (!maybe_mesh_id) return false;
            mesh_id = maybe_mesh_id;
            break;
        }
        default: {
            throw std::runtime_error(
                fmt::format("unknown mesh enum value: {}",
                            static_cast<int>(proto_scene_static_mesh.mesh_enum())));
        }
    }
    auto maybe_material_id = level.GetIdFromName(proto_scene_static_mesh.material_name());
    if (!maybe_material_id) return false;
    const EntityId material_id = maybe_material_id;
    auto& mesh                 = level.GetStaticMeshFromId(mesh_id);
    mesh.SetRenderPrimitive(proto_scene_static_mesh.render_primitive_enum());
    std::unique_ptr<NodeInterface> node_interface =
        std::make_unique<NodeStaticMesh>(GetFunctor(level), mesh_id);
    node_interface->SetName(proto_scene_static_mesh.name());
    node_interface->SetParentName(proto_scene_static_mesh.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(node_interface));
    level.AddMeshMaterialId(maybe_scene_id, material_id);
    if (!maybe_scene_id) throw std::runtime_error("No scene Id.");
    return true;
}

[[nodiscard]] bool ParseSceneStaticMeshFileName(LevelInterface& level,
                                                const SceneStaticMesh& proto_scene_static_mesh) {
    auto vec_node_mesh_id = opengl::file::LoadStaticMeshesFromFile(
        level, "asset/model/" + proto_scene_static_mesh.file_name(), proto_scene_static_mesh.name(),
        proto_scene_static_mesh.material_name());
    if (vec_node_mesh_id.empty()) return false;
    int i = 0;
    for (const auto node_mesh_id : vec_node_mesh_id) {
        auto& node = level.GetSceneNodeFromId(node_mesh_id);
        auto& mesh = level.GetStaticMeshFromId(node.GetLocalMesh());
        mesh.SetRenderPrimitive(proto_scene_static_mesh.render_primitive_enum());
        auto str = fmt::format("{}.{}", proto_scene_static_mesh.name(), i);
        mesh.SetName(str);
        node.SetParentName(proto_scene_static_mesh.parent());
        ++i;
    }
    return true;
}

[[nodiscard]] bool ParseSceneStaticMeshStreamInput(LevelInterface& level,
                                                   const SceneStaticMesh& proto_scene_static_mesh) {
    assert(proto_scene_static_mesh.has_multi_plugin());
    auto point_buffer = std::make_unique<opengl::Buffer>(opengl::BufferTypeEnum::ARRAY_BUFFER,
                                                         opengl::BufferUsageEnum::STREAM_DRAW);
    point_buffer->SetName("point." + proto_scene_static_mesh.name());
    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer   = std::make_unique<opengl::Buffer>(opengl::BufferTypeEnum::ARRAY_BUFFER,
                                                          opengl::BufferUsageEnum::STREAM_DRAW);
    normal_buffer->SetName("normal." + proto_scene_static_mesh.name());
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto index_buffer     = std::make_unique<opengl::Buffer>(
        opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER, opengl::BufferUsageEnum::STREAM_DRAW);
    index_buffer->SetName("index." + proto_scene_static_mesh.name());
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));
    auto color_buffer    = std::make_unique<opengl::Buffer>(opengl::BufferTypeEnum::ARRAY_BUFFER,
                                                         opengl::BufferUsageEnum::STREAM_DRAW);
    color_buffer->SetName("color." + proto_scene_static_mesh.name());
    auto color_buffer_id = level.AddBuffer(std::move(color_buffer));

    // Create a new static mesh.
    StaticMeshParameter parameter   = {};
    parameter.point_buffer_id       = point_buffer_id;
    parameter.normal_buffer_id      = normal_buffer_id;
    parameter.color_buffer_id       = color_buffer_id;
    parameter.index_buffer_id       = index_buffer_id;
    parameter.render_primitive_enum = proto::SceneStaticMesh::POINT;
    auto mesh                       = std::make_unique<opengl::StaticMesh>(level, parameter);
    mesh->SetName("mesh." + proto_scene_static_mesh.name());
    auto mesh_id = level.AddStaticMesh(std::move(mesh));

    // Get the material id.
    auto maybe_material_id = level.GetIdFromName(proto_scene_static_mesh.material_name());
    if (!maybe_material_id) return false;
    const EntityId material_id = maybe_material_id;

    // Create the node corresponding to the mesh.
    auto& mesh_ref = level.GetStaticMeshFromId(mesh_id);
    mesh_ref.SetRenderPrimitive(proto_scene_static_mesh.render_primitive_enum());
    std::unique_ptr<NodeInterface> node_interface =
        std::make_unique<NodeStaticMesh>(GetFunctor(level), mesh_id);
    node_interface->SetName(proto_scene_static_mesh.name());
    node_interface->SetParentName(proto_scene_static_mesh.parent());
    auto scene_id = level.AddSceneNode(std::move(node_interface));
    level.AddMeshMaterialId(scene_id, material_id);
    if (!scene_id) throw std::runtime_error("No scene Id.");
    return true;
}

[[nodiscard]] bool ParseSceneStaticMesh(LevelInterface& level,
                                        const SceneStaticMesh& proto_scene_static_mesh) {
    // 1st case this is a clean static mesh node.
    if (proto_scene_static_mesh.has_clean_buffer()) {
        return ParseSceneStaticMeshClearBuffer(level, proto_scene_static_mesh);
    }
    // 2nd case this is a enum static mesh node (CUBE or QUAD).
    if (proto_scene_static_mesh.has_mesh_enum()) {
        return ParseSceneStaticMeshMeshEnum(level, proto_scene_static_mesh);
    }
    // 3rd case this is a mesh file.
    if (proto_scene_static_mesh.has_file_name()) {
        return ParseSceneStaticMeshFileName(level, proto_scene_static_mesh);
    }
    // 4th case stream input.
    if (proto_scene_static_mesh.has_multi_plugin()) {
        return ParseSceneStaticMeshStreamInput(level, proto_scene_static_mesh);
    }
    return false;
}

[[nodiscard]] bool ParseSceneCamera(LevelInterface& level,
                                    const frame::proto::SceneCamera& proto_scene_camera) {
    if (proto_scene_camera.fov_degrees() == 0.0) {
        throw std::runtime_error("Need field of view degrees in camera.");
    }
    std::unique_ptr<NodeInterface> scene_camera = std::make_unique<NodeCamera>(
        GetFunctor(level), ParseUniform(proto_scene_camera.position()),
        ParseUniform(proto_scene_camera.target()), ParseUniform(proto_scene_camera.up()),
        proto_scene_camera.fov_degrees(), proto_scene_camera.aspect_ratio(),
        proto_scene_camera.near_clip(), proto_scene_camera.far_clip());
    scene_camera->SetName(proto_scene_camera.name());
    scene_camera->SetParentName(proto_scene_camera.parent());
    auto maybe_scene_id = level.AddSceneNode(std::move(scene_camera));
    return static_cast<bool>(maybe_scene_id);
}

[[nodiscard]] bool ParseSceneLight(LevelInterface& level,
                                   const proto::SceneLight& proto_scene_light) {
    switch (proto_scene_light.light_type()) {
        case proto::SceneLight::POINT: {
            std::unique_ptr<NodeInterface> node_light = std::make_unique<frame::NodeLight>(
                GetFunctor(level), NodeLightEnum::POINT, ParseUniform(proto_scene_light.position()),
                ParseUniform(proto_scene_light.color()));
            auto maybe_node_id = level.AddSceneNode(std::move(node_light));
            return static_cast<bool>(maybe_node_id);
        }
        case proto::SceneLight::DIRECTIONAL: {
            std::unique_ptr<NodeInterface> node_light =
                std::make_unique<frame::NodeLight>(GetFunctor(level), NodeLightEnum::DIRECTIONAL,
                                                   ParseUniform(proto_scene_light.direction()),
                                                   ParseUniform(proto_scene_light.color()));
            auto maybe_node_id = level.AddSceneNode(std::move(node_light));
            return static_cast<bool>(maybe_node_id);
        }
        case proto::SceneLight::AMBIENT:
            [[fallthrough]];
        case proto::SceneLight::SPOT:
            [[fallthrough]];
        case proto::SceneLight::INVALID:
            [[fallthrough]];
        default:
            throw std::runtime_error(fmt::format("Unknown scene light type {}",
                                                 static_cast<int>(proto_scene_light.light_type())));
    }
    return false;
}

}  // End namespace.

[[nodiscard]] bool ParseSceneTreeFile(const SceneTree& proto_scene_tree, LevelInterface& level) {
    level.SetDefaultCameraName(proto_scene_tree.default_camera_name());
    level.SetDefaultRootSceneNodeName(proto_scene_tree.default_root_name());
    for (const auto& proto_matrix : proto_scene_tree.scene_matrices()) {
        if (!ParseSceneMatrix(level, proto_matrix)) return false;
    }
    for (const auto& proto_static_mesh : proto_scene_tree.scene_static_meshes()) {
        if (!ParseSceneStaticMesh(level, proto_static_mesh)) return false;
    }
    for (const auto& proto_camera : proto_scene_tree.scene_cameras()) {
        if (!ParseSceneCamera(level, proto_camera)) return false;
    }
    for (const auto& proto_light : proto_scene_tree.scene_lights()) {
        if (!ParseSceneLight(level, proto_light)) return false;
    }
    return true;
}

}  // End namespace frame::proto.
