#include "frame/vulkan/json/parse_scene_tree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numeric>
#include <stdexcept>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "frame/json/parse_uniform.h"
#include "frame/logger.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/file/file_system.h"
#include "frame/file/obj.h"
#include "frame/bvh.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/static_mesh.h"

namespace frame::vulkan::json
{

namespace
{

struct GpuBvhNode
{
    glm::vec4 min;
    glm::vec4 max;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

static_assert(sizeof(GpuBvhNode) == 48);

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
    node->GetData().set_acceleration_structure_enum(
        proto_mesh.acceleration_structure_enum());

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
    node->GetData().set_acceleration_structure_enum(
        proto_mesh.acceleration_structure_enum());
    auto scene_id = level.AddSceneNode(std::move(node));
    level.AddMeshMaterialId(
        scene_id, frame::NullId, proto_mesh.render_time_enum());
    return true;
}

bool ParseNodeStaticMesh(
    LevelInterface& level,
    const frame::proto::NodeStaticMesh& proto_mesh)
{
    auto material_id = level.GetIdFromName(proto_mesh.material_name());
    if (!material_id && !proto_mesh.material_name().empty())
    {
        throw std::runtime_error(std::format(
            "Material {} not found for static mesh {}.",
            proto_mesh.material_name(),
            proto_mesh.name()));
    }

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
        const auto asset_root = frame::file::FindDirectory("asset");
        const auto path = (asset_root / "model" / proto_mesh.file_name())
                              .lexically_normal();
        frame::file::Obj obj(path);

        const bool build_bvh =
            proto_mesh.acceleration_structure_enum() ==
            frame::proto::NodeStaticMesh::BVH_ACCELERATION;
        int counter = 0;
        for (const auto& mesh : obj.GetMeshes())
        {
            std::vector<float> points;
            std::vector<float> normals;
            std::vector<float> textures;
            const auto& vertices = mesh.GetVertices();
            points.reserve(vertices.size() * 3);
            normals.reserve(vertices.size() * 3);
            textures.reserve(vertices.size() * 2);
            std::size_t invalid_uvs = 0;
            for (const auto& vertex : vertices)
            {
                points.push_back(vertex.point.x);
                points.push_back(vertex.point.y);
                points.push_back(vertex.point.z);
                normals.push_back(vertex.normal.x);
                normals.push_back(vertex.normal.y);
                normals.push_back(vertex.normal.z);
                if (mesh.HasTextureCoordinates())
                {
                    float u = vertex.tex_coord.x;
                    float v = vertex.tex_coord.y;
                    if (!std::isfinite(u) || !std::isfinite(v))
                    {
                        u = 0.0f;
                        v = 0.0f;
                        ++invalid_uvs;
                    }
                    textures.push_back(u);
                    textures.push_back(v);
                }
                else
                {
                    textures.push_back(vertex.normal.x);
                    textures.push_back(vertex.normal.y);
                }
            }
            if (invalid_uvs > 0)
            {
                frame::Logger::GetInstance()->warn(
                    "Vulkan mesh {} had {} invalid UVs; clamped to 0.",
                    proto_mesh.name(),
                    invalid_uvs);
            }

            std::vector<std::uint32_t> indices;
            indices.reserve(mesh.GetIndices().size());
            for (int idx : mesh.GetIndices())
            {
                indices.push_back(static_cast<std::uint32_t>(idx));
            }
            const std::size_t vertex_count = points.size() / 3;
            if (!indices.empty() && vertex_count > 0)
            {
                const auto [min_it, max_it] =
                    std::minmax_element(indices.begin(), indices.end());
                const bool has_zero =
                    std::find(indices.begin(), indices.end(), 0) !=
                    indices.end();
                const bool looks_one_based =
                    !has_zero &&
                    *min_it >= 1 &&
                    *max_it == vertex_count;
                if (looks_one_based)
                {
                    for (auto& idx : indices)
                    {
                        idx -= 1;
                    }
                }
                const auto [min_it_after, max_it_after] =
                    std::minmax_element(indices.begin(), indices.end());
                frame::Logger::GetInstance()->info(
                    "Vulkan mesh {}: vertices={}, indices={}, min_idx={}, max_idx={}, has_zero={}, one_based={}",
                    proto_mesh.name(),
                    vertex_count,
                    indices.size(),
                    static_cast<std::uint32_t>(*min_it_after),
                    static_cast<std::uint32_t>(*max_it_after),
                    has_zero ? "true" : "false",
                    looks_one_based ? "true" : "false");
                if (indices.size() >= 9)
                {
                    frame::Logger::GetInstance()->info(
                        "Vulkan mesh {} first indices: {} {} {} | {} {} {} | {} {} {}",
                        proto_mesh.name(),
                        indices[0], indices[1], indices[2],
                        indices[3], indices[4], indices[5],
                        indices[6], indices[7], indices[8]);
                }
            }
            const auto indices_valid =
                !indices.empty() &&
                std::all_of(
                    indices.begin(),
                    indices.end(),
                    [vertex_count](std::uint32_t idx) {
                        return idx < vertex_count;
                    }) &&
                (indices.size() % 3 == 0);
            std::vector<std::uint32_t> fallback_indices;
            const std::vector<std::uint32_t>* triangle_indices = &indices;
            if (!indices_valid && vertex_count > 0)
            {
                fallback_indices.resize(vertex_count);
                std::iota(fallback_indices.begin(), fallback_indices.end(), 0);
                const auto remainder = fallback_indices.size() % 3;
                if (remainder != 0)
                {
                    fallback_indices.resize(
                        fallback_indices.size() - remainder);
                }
                triangle_indices = &fallback_indices;
            }

            auto make_buffer = [](const auto& data,
                                  const std::string& name,
                                  LevelInterface& lvl) -> EntityId {
                if (data.empty())
                {
                    return NullId;
                }
                auto buffer = std::make_unique<frame::vulkan::Buffer>();
                buffer->Copy(data.size() * sizeof(data[0]), data.data());
                buffer->SetName(name);
                return lvl.AddBuffer(std::move(buffer));
            };

            auto point_buffer_id = make_buffer(
                points,
                std::format("{}.{}.point", proto_mesh.name(), counter),
                level);
            if (!point_buffer_id)
            {
                throw std::runtime_error("Failed to create point buffer.");
            }
            auto normal_buffer_id = make_buffer(
                normals,
                std::format("{}.{}.normal", proto_mesh.name(), counter),
                level);
            auto texture_buffer_id = make_buffer(
                textures,
                std::format("{}.{}.texture", proto_mesh.name(), counter),
                level);
            auto index_buffer_id = make_buffer(
                indices,
                std::format("{}.{}.index", proto_mesh.name(), counter),
                level);
            if (!index_buffer_id)
            {
                throw std::runtime_error("Failed to create index buffer.");
            }

            std::vector<float> triangles;
            triangles.reserve(triangle_indices->size() * 12);
            auto push_vertex = [&](std::uint32_t idx) {
                const auto base = idx * 3;
                const auto uv_base = idx * 2;
                // Position
                triangles.push_back(points[base + 0]);
                triangles.push_back(points[base + 1]);
                triangles.push_back(points[base + 2]);
                triangles.push_back(0.0f); // Padding
                // Normal
                if (!normals.empty())
                {
                    triangles.push_back(normals[base + 0]);
                    triangles.push_back(normals[base + 1]);
                    triangles.push_back(normals[base + 2]);
                }
                else
                {
                    triangles.push_back(0.0f);
                    triangles.push_back(0.0f);
                    triangles.push_back(0.0f);
                }
                triangles.push_back(0.0f); // Padding
                // UV
                if (!textures.empty())
                {
                    triangles.push_back(textures[uv_base + 0]);
                    triangles.push_back(textures[uv_base + 1]);
                }
                else
                {
                    triangles.push_back(0.0f);
                    triangles.push_back(0.0f);
                }
                triangles.push_back(0.0f); // Padding
                triangles.push_back(0.0f); // Padding
            };
            for (std::size_t i = 0;
                 i + 2 < triangle_indices->size();
                 i += 3)
            {
                push_vertex((*triangle_indices)[i]);
                push_vertex((*triangle_indices)[i + 1]);
                push_vertex((*triangle_indices)[i + 2]);
            }

            auto triangle_buffer_id = make_buffer(
                triangles,
                std::format("{}.{}.triangle", proto_mesh.name(), counter),
                level);
            EntityId bvh_buffer_id = NullId;
            if (build_bvh)
            {
                auto bvh_nodes = frame::BuildBVH(points, *triangle_indices);
                std::vector<GpuBvhNode> gpu_bvh_nodes;
                gpu_bvh_nodes.reserve(bvh_nodes.size());
                for (const auto& node : bvh_nodes)
                {
                    GpuBvhNode gpu_node{};
                    gpu_node.min = glm::vec4(node.min, 0.0f);
                    gpu_node.max = glm::vec4(node.max, 0.0f);
                    gpu_node.left = node.left;
                    gpu_node.right = node.right;
                    gpu_node.first_triangle = node.first_triangle;
                    gpu_node.triangle_count = node.triangle_count;
                    gpu_bvh_nodes.push_back(gpu_node);
                }
                bvh_buffer_id = make_buffer(
                    gpu_bvh_nodes,
                    std::format("{}.{}.bvh", proto_mesh.name(), counter),
                    level);
            }

            frame::StaticMeshParameter parameter{};
            parameter.point_buffer_id = point_buffer_id;
            parameter.normal_buffer_id = normal_buffer_id;
            parameter.texture_buffer_id = texture_buffer_id;
            parameter.index_buffer_id = index_buffer_id;
            parameter.triangle_buffer_id = triangle_buffer_id;
            parameter.bvh_buffer_id = bvh_buffer_id;
            parameter.render_primitive_enum =
                proto_mesh.render_primitive_enum();

            auto static_mesh =
                std::make_unique<frame::vulkan::StaticMesh>(parameter, true);
            static_mesh->SetIndexSize(indices.size() * sizeof(std::uint32_t));
            const std::string mesh_name =
                std::format("{}.{}.mesh", proto_mesh.name(), counter);
            static_mesh->SetName(mesh_name);
            static_mesh->GetData().set_file_name(proto_mesh.file_name());
            static_mesh->GetData().set_render_primitive_enum(
                proto_mesh.render_primitive_enum());
            static_mesh->GetData().set_acceleration_structure_enum(
                proto_mesh.acceleration_structure_enum());
            auto mesh_id = level.AddStaticMesh(std::move(static_mesh));
            if (!mesh_id)
            {
                throw std::runtime_error("Failed to add static mesh to level.");
            }

            auto node = std::make_unique<frame::NodeStaticMesh>(
                MakeResolver(level), mesh_id);
            std::string node_name = (obj.GetMeshes().size() == 1)
                                        ? proto_mesh.name()
                                        : std::format(
                                              "{}.{}",
                                              proto_mesh.name(),
                                              counter);
            node->SetName(node_name);
            node->SetParentName(proto_mesh.parent());
            node->GetData().set_material_name(proto_mesh.material_name());
            node->GetData().set_render_time_enum(proto_mesh.render_time_enum());
            node->GetData().set_acceleration_structure_enum(
                proto_mesh.acceleration_structure_enum());
            node->GetData().set_file_name(proto_mesh.file_name());

            auto scene_id = level.AddSceneNode(std::move(node));
            level.AddMeshMaterialId(
                scene_id, material_id, proto_mesh.render_time_enum());
            ++counter;
        }
        return true;
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
