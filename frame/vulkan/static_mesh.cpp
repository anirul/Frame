#include "frame/vulkan/static_mesh.h"

#include <cstdint>
#include <format>
#include <memory>
#include <numeric>
#include <vector>

#include "frame/level_interface.h"
#include "frame/vulkan/buffer.h"

namespace frame::vulkan
{

StaticMesh::StaticMesh(
    const frame::StaticMeshParameter& parameters, bool clear_buffer)
    : parameter_(parameters), clear_buffer_(clear_buffer)
{
    index_size_ = 0;
    auto& proto = GetData();
    proto.set_render_primitive_enum(parameters.render_primitive_enum);
    proto.set_shadow_effect_enum(parameters.shadow_effect_enum);
}

namespace
{

std::unique_ptr<frame::BufferInterface> MakeBuffer(
    const std::vector<float>& data)
{
    auto buffer = std::make_unique<Buffer>();
    buffer->Copy(data);
    return buffer;
}

std::unique_ptr<frame::BufferInterface> MakeBuffer(
    const std::vector<std::uint32_t>& data)
{
    auto buffer = std::make_unique<Buffer>();
    buffer->Copy(data);
    return buffer;
}

} // namespace

frame::EntityId CreateQuadStaticMesh(frame::LevelInterface& level)
{
    std::vector<float> points = {
        -1.f, 1.f, 0.f, 1.f, 1.f, 0.f, -1.f, -1.f, 0.f, 1.f, -1.f, 0.f};
    std::vector<float> normals = {
        0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
    std::vector<float> textures = {0, 1, 1, 1, 0, 0, 1, 0};
    std::vector<std::uint32_t> indices = {0, 1, 2, 1, 3, 2};

    static std::int64_t count = 0;
    ++count;

    auto point_buffer = MakeBuffer(points);
    auto normal_buffer = MakeBuffer(normals);
    auto texture_buffer = MakeBuffer(textures);
    auto index_buffer = MakeBuffer(indices);

    point_buffer->SetName(std::format("QuadPoint.{}", count));
    normal_buffer->SetName(std::format("QuadNormal.{}", count));
    texture_buffer->SetName(std::format("QuadTexture.{}", count));
    index_buffer->SetName(std::format("QuadIndex.{}", count));

    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto texture_buffer_id = level.AddBuffer(std::move(texture_buffer));
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));

    frame::StaticMeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.texture_buffer_id = texture_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.render_primitive_enum =
        frame::proto::NodeStaticMesh::TRIANGLE_PRIMITIVE;

    auto mesh = std::make_unique<StaticMesh>(parameter, true);
    mesh->SetName(std::format("QuadMesh.{}", count));
    mesh->SetIndexSize(indices.size() * sizeof(std::uint32_t));
    return level.AddStaticMesh(std::move(mesh));
}

frame::EntityId CreateCubeStaticMesh(frame::LevelInterface& level)
{
    std::vector<float> points = {
        // Front.
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        // Back.
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        // Left.
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        // Right.
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        // Bottom.
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
        // Top.
        -0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f};

    std::vector<float> normals = {
        // Front.
        0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f,
        0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f,
        // Back.
        0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,
        0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,
        // Left.
        -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,
        -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,
        // Right.
        1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,
        1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,
        // Bottom.
        0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,
        0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,  0.f,  -1.f, 0.f,
        // Top.
        0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f,
        0.f,  1.f,  0.f,  0.f,  1.f,  0.f,  0.f,  1.f,  0.f};

    std::vector<float> textures = {
        // Front.
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        // Back.
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        // Left.
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        // Right.
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        // Bottom.
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        // Top.
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f};

    std::vector<std::uint32_t> indices(36);
    std::iota(indices.begin(), indices.end(), 0);

    static std::int64_t count = 0;
    ++count;

    auto point_buffer = MakeBuffer(points);
    auto normal_buffer = MakeBuffer(normals);
    auto texture_buffer = MakeBuffer(textures);
    auto index_buffer = MakeBuffer(indices);

    point_buffer->SetName(std::format("CubePoint.{}", count));
    normal_buffer->SetName(std::format("CubeNormal.{}", count));
    texture_buffer->SetName(std::format("CubeTexture.{}", count));
    index_buffer->SetName(std::format("CubeIndex.{}", count));

    auto point_buffer_id = level.AddBuffer(std::move(point_buffer));
    auto normal_buffer_id = level.AddBuffer(std::move(normal_buffer));
    auto texture_buffer_id = level.AddBuffer(std::move(texture_buffer));
    auto index_buffer_id = level.AddBuffer(std::move(index_buffer));

    frame::StaticMeshParameter parameter = {};
    parameter.point_buffer_id = point_buffer_id;
    parameter.normal_buffer_id = normal_buffer_id;
    parameter.texture_buffer_id = texture_buffer_id;
    parameter.index_buffer_id = index_buffer_id;
    parameter.render_primitive_enum =
        frame::proto::NodeStaticMesh::TRIANGLE_PRIMITIVE;

    auto mesh = std::make_unique<StaticMesh>(parameter, true);
    mesh->SetName(std::format("CubeMesh.{}", count));
    mesh->SetIndexSize(indices.size() * sizeof(std::uint32_t));
    return level.AddStaticMesh(std::move(mesh));
}

} // namespace frame::vulkan
