#include "frame/opengl/static_mesh.h"

#include <GL/glew.h>

#include <fstream>
#include <iterator>
#include <numeric>
#include <sstream>

namespace frame::opengl {

StaticMesh::StaticMesh(LevelInterface* level, const StaticMeshParameter& parameter)
    : level_(level),
      point_buffer_id_(parameter.point_buffer_id),
      point_buffer_size_(parameter.point_buffer_size),
      color_buffer_id_(parameter.color_buffer_id),
      color_buffer_size_(parameter.color_buffer_size),
      normal_buffer_id_(parameter.normal_buffer_id),
      normal_buffer_size_(parameter.normal_buffer_size),
      texture_buffer_id_(parameter.texture_buffer_id),
      texture_buffer_size_(parameter.texture_buffer_size),
      index_buffer_id_(parameter.index_buffer_id),
      render_primitive_enum_(parameter.render_primitive_enum) {
    if (!point_buffer_id_) throw std::runtime_error("No point buffer specified.");
    // Create a new vertex array (to render the mesh).
    glGenVertexArrays(1, &vertex_array_object_);
    glBindVertexArray(vertex_array_object_);

    // Point buffer.
    auto* point_buffer_ptr = dynamic_cast<Buffer*>(level_->GetBufferFromId(point_buffer_id_));
    assert(point_buffer_ptr);
    // Create the point buffer.
    point_buffer_ptr->Bind();
    glVertexAttribPointer(0, point_buffer_size_, GL_FLOAT, GL_FALSE, 0, nullptr);
    point_buffer_ptr->UnBind();

    // Counter of array buffers.
    std::uint32_t vertex_array_count = 0;
    static std::uint32_t count       = 0;
    std::size_t point_size_element   = point_buffer_ptr->GetSize() / sizeof(float);

    // Color buffer.
    if (color_buffer_id_) {
        auto* gl_color_buffer = dynamic_cast<Buffer*>(level->GetBufferFromId(color_buffer_id_));
        assert(gl_color_buffer);
        gl_color_buffer->Bind();
        glVertexAttribPointer(++vertex_array_count, color_buffer_size_, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
        gl_color_buffer->UnBind();
    } else if (std::count(parameter.generate_list.begin(), parameter.generate_list.end(),
                          StaticMeshParameter::StaticMeshParameterEnum::GENERATE_COLOR)) {
        std::vector<float> color;
        color.resize(point_size_element);
        std::fill(color.begin(), color.end(), 1.0f);
        std::unique_ptr<BufferInterface> gl_color_buffer = std::make_unique<Buffer>();
        gl_color_buffer->SetName(fmt::format("Mesh.Buffer.Color.{}", count));
        gl_color_buffer->Copy(color);
        color_buffer_id_       = level_->AddBuffer(std::move(gl_color_buffer));
        auto* color_buffer_ptr = dynamic_cast<Buffer*>(level_->GetBufferFromId(color_buffer_id_));
        assert(color_buffer_ptr);
        color_buffer_ptr->Bind();
        glVertexAttribPointer(++vertex_array_count, color_buffer_size_, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
        color_buffer_ptr->UnBind();
    }

    // Normal buffer.
    if (normal_buffer_id_) {
        auto* gl_normal_buffer = dynamic_cast<Buffer*>(level->GetBufferFromId(normal_buffer_id_));
        assert(gl_normal_buffer);
        gl_normal_buffer->Bind();
        glVertexAttribPointer(++vertex_array_count, normal_buffer_size_, GL_FLOAT, GL_TRUE, 0,
                              nullptr);
        gl_normal_buffer->UnBind();
    } else if (std::count(parameter.generate_list.begin(), parameter.generate_list.end(),
                          StaticMeshParameter::StaticMeshParameterEnum::GENERATE_NORMAL)) {
        std::vector<float> normal;
        normal.resize(point_size_element);
        std::fill(normal.begin(), normal.end(), 0.0f);
        for (int i = 0; i < point_size_element; i += 3) {
            normal[i] = -1.0f;
        }
        std::unique_ptr<BufferInterface> gl_normal_buffer = std::make_unique<Buffer>();
        gl_normal_buffer->SetName(fmt::format("Mesh.Buffer.Normal.{}", count));
        gl_normal_buffer->Copy(normal);
        normal_buffer_id_       = level_->AddBuffer(std::move(gl_normal_buffer));
        auto* normal_buffer_ptr = dynamic_cast<Buffer*>(level_->GetBufferFromId(normal_buffer_id_));
        assert(normal_buffer_ptr);
        normal_buffer_ptr->Bind();
        glVertexAttribPointer(++vertex_array_count, normal_buffer_size_, GL_FLOAT, GL_TRUE, 0,
                              nullptr);
        normal_buffer_ptr->UnBind();
    }

    // Texture coordinate buffer.
    if (texture_buffer_id_) {
        auto* gl_texture_buffer = dynamic_cast<Buffer*>(level->GetBufferFromId(texture_buffer_id_));
        assert(gl_texture_buffer);
        gl_texture_buffer->Bind();
        glVertexAttribPointer(++vertex_array_count, texture_buffer_size_, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
        gl_texture_buffer->UnBind();
    } else if (std::count(
                   parameter.generate_list.begin(), parameter.generate_list.end(),
                   StaticMeshParameter::StaticMeshParameterEnum::GENERATE_TEXTURE_COORDINATE)) {
        std::vector<float> texture_coordinate;
        std::size_t texture_coordinate_size =
            static_cast<std::size_t>(point_size_element * 2.0 / 3.0);
        texture_coordinate.resize(texture_coordinate_size);
        std::fill(texture_coordinate.begin(), texture_coordinate.end(), 0.5f);
        std::unique_ptr<BufferInterface> gl_texture_coordinate = std::make_unique<Buffer>();
        gl_texture_coordinate->SetName(fmt::format("Mesh.Buffer.TexCoord.{}", count));
        gl_texture_coordinate->Copy(texture_coordinate);
        texture_buffer_id_ = level_->AddBuffer(std::move(gl_texture_coordinate));
        auto* texture_buffer_ptr =
            dynamic_cast<Buffer*>(level_->GetBufferFromId(texture_buffer_id_));
        assert(texture_buffer_ptr);
        texture_buffer_ptr->Bind();
        glVertexAttribPointer(++vertex_array_count, texture_buffer_size_, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
        texture_buffer_ptr->UnBind();
    }

    // Index buffer.
    if (!index_buffer_id_) {
        if (render_primitive_enum_ != proto::SceneStaticMesh::POINT) {
            throw std::runtime_error("No index buffer and render type is not set to point.");
        }
        if (!std::count(parameter.generate_list.begin(), parameter.generate_list.end(),
                        StaticMeshParameter::StaticMeshParameterEnum::GENERATE_INDEX)) {
            throw std::runtime_error("No GENERATE_INDEX in the generate list.");
        }
        std::size_t index_size_element = point_size_element / 3;
        index_size_                    = index_size_element * sizeof(std::uint32_t);
        std::vector<std::uint32_t> index;
        index.resize(index_size_element);
        std::iota(index.begin(), index.end(), 0);
        std::unique_ptr<BufferInterface> gl_index_buffer = std::make_unique<Buffer>(
            BufferTypeEnum::ELEMENT_ARRAY_BUFFER, BufferUsageEnum::STREAM_DRAW);
        gl_index_buffer->SetName(fmt::format("Mesh.Buffer.Index.{}", count));
        gl_index_buffer->Copy(index);
        index_buffer_id_ = level_->AddBuffer(std::move(gl_index_buffer));
    } else {
        index_size_ = level_->GetBufferFromId(index_buffer_id_)->GetSize();
    }

    // Increment static counter.
    count++;
    SetRenderPrimitive(render_primitive_enum_);

    // Enable vertex attrib array.
    for (std::uint32_t i = 0; i <= vertex_array_count; ++i) {
        glEnableVertexAttribArray(i);
    }
    glBindVertexArray(0);
}

StaticMesh::~StaticMesh() {
    glDeleteVertexArrays(1, &vertex_array_object_);
    // Try to delete assigned buffers.
    if (point_buffer_id_) {
        level_->RemoveBuffer(point_buffer_id_);
    }
    if (color_buffer_id_) {
        level_->RemoveBuffer(color_buffer_id_);
    }
    if (normal_buffer_id_) {
        level_->RemoveBuffer(normal_buffer_id_);
    }
    if (texture_buffer_id_) {
        level_->RemoveBuffer(texture_buffer_id_);
    }
    if (index_buffer_id_) {
        level_->RemoveBuffer(index_buffer_id_);
    }
}

void StaticMesh::Bind(const unsigned int slot /*= 0*/) const {
    if (locked_bind_) return;
    glBindVertexArray(vertex_array_object_);
}

void StaticMesh::UnBind() const {
    if (locked_bind_) return;
    glBindVertexArray(0);
}

EntityId CreateQuadStaticMesh(LevelInterface* level) {
    std::vector<float> points = {
        -1.f, 1.f, 0.f, 1.f, 1.f, 0.f, -1.f, -1.f, 0.f, 1.f, -1.f, 0.f,
    };
    std::vector<float> normals = {
        0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
    };
    std::vector<float> textures = {
        0, 1, 1, 1, 0, 0, 1, 0,
    };
    std::vector<std::uint32_t> indices = {
        0, 1, 2, 1, 3, 2,
    };
    auto point_buffer   = std::make_unique<Buffer>();
    auto normal_buffer  = std::make_unique<Buffer>();
    auto texture_buffer = std::make_unique<Buffer>();
    auto index_buffer   = std::make_unique<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
    point_buffer->Copy(points);
    normal_buffer->Copy(normals);
    texture_buffer->Copy(textures);
    index_buffer->Copy(indices);
    static std::int64_t count = 0;
    count++;
    point_buffer->SetName(fmt::format("QuadPoint.{}", count));
    normal_buffer->SetName(fmt::format("QuadNormal.{}", count));
    texture_buffer->SetName(fmt::format("QuadTexture.{}", count));
    index_buffer->SetName(fmt::format("QuadIndex.{}", count));
    auto maybe_point_buffer_id = level->AddBuffer(std::move(point_buffer));
    if (!maybe_point_buffer_id) return NullId;
    auto maybe_normal_buffer_id = level->AddBuffer(std::move(normal_buffer));
    if (!maybe_normal_buffer_id) return NullId;
    auto maybe_texture_buffer_id = level->AddBuffer(std::move(texture_buffer));
    if (!maybe_texture_buffer_id) return NullId;
    auto maybe_index_buffer_id = level->AddBuffer(std::move(index_buffer));
    if (!maybe_index_buffer_id) return NullId;
    StaticMeshParameter parameter   = {};
    parameter.point_buffer_id       = maybe_point_buffer_id;
    parameter.normal_buffer_id      = maybe_normal_buffer_id;
    parameter.texture_buffer_id     = maybe_texture_buffer_id;
    parameter.index_buffer_id       = maybe_index_buffer_id;
    parameter.render_primitive_enum = proto::SceneStaticMesh::TRIANGLE;
    auto mesh                       = std::make_unique<StaticMesh>(level, parameter);
    mesh->SetName(fmt::format("QuadMesh.{}", count));
    return level->AddStaticMesh(std::move(mesh));
}

EntityId CreateCubeStaticMesh(LevelInterface* level) {
    // Create a cube but multiply the size, so we can index it by iota.
    std::vector<float> points = {
        // clang-format off
		// Face front.
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		// Face back
		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		// Face left.
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		// Face right.
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		// Face bottom.
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,
		// Face top.
		-0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
        // clang-format on
    };
    std::vector<float> normals = {
        // clang-format off
		// Face front.
		.0f, .0f, -1.f,
		.0f, .0f, -1.f,
		.0f, .0f, -1.f,
		.0f, .0f, -1.f,
		.0f, .0f, -1.f,
		.0f, .0f, -1.f,
        // Face back.
		.0f, .0f, 1.f,
		.0f, .0f, 1.f,
		.0f, .0f, 1.f,
		.0f, .0f, 1.f,
		.0f, .0f, 1.f,
		.0f, .0f, 1.f,
		// Face left.
		-1.f, .0f, .0f,
		-1.f, .0f, .0f,
		-1.f, .0f, .0f,
		-1.f, .0f, .0f,
		-1.f, .0f, .0f,
		-1.f, .0f, .0f,
		// Face right.
		1.f, .0f, .0f,
		1.f, .0f, .0f,
		1.f, .0f, .0f,
		1.f, .0f, .0f,
		1.f, .0f, .0f,
		1.f, .0f, .0f,
		// Face bottom.
		.0f, -1.f, -.0f,
		.0f, -1.f, -.0f,
		.0f, -1.f, -.0f,
		.0f, -1.f, -.0f,
		.0f, -1.f, -.0f,
		.0f, -1.f, -.0f,
		// Face top.
		.0f, 1.f, 0.f,
		.0f, 1.f, 0.f,
		.0f, 1.f, 0.f,
		.0f, 1.f, 0.f,
		.0f, 1.f, 0.f,
		.0f, 1.f, 0.f,
        // clang-format on
    };
    std::vector<float> textures = {
        // clang-format off
		// Face front.
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		// Face back.
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		// Face left.
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		// Face right.
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		// Face bottom.
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		// Face top.
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
        // clang-format on
    };
    std::vector<std::uint32_t> indices;
    indices.resize(18 * 3);
    std::iota(indices.begin(), indices.end(), 0);
    auto point_buffer   = std::make_unique<Buffer>();
    auto normal_buffer  = std::make_unique<Buffer>();
    auto texture_buffer = std::make_unique<Buffer>();
    auto index_buffer   = std::make_unique<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
    point_buffer->Copy(points);
    normal_buffer->Copy(normals);
    texture_buffer->Copy(textures);
    index_buffer->Copy(indices);
    static std::int64_t count = 0;
    count++;
    point_buffer->SetName(fmt::format("CubePoint.{}", count));
    normal_buffer->SetName(fmt::format("CubeNormal.{}", count));
    texture_buffer->SetName(fmt::format("CubeTexture.{}", count));
    index_buffer->SetName(fmt::format("CubeIndex.{}", count));
    auto maybe_point_buffer_id = level->AddBuffer(std::move(point_buffer));
    if (!maybe_point_buffer_id) return NullId;
    auto maybe_normal_buffer_id = level->AddBuffer(std::move(normal_buffer));
    if (!maybe_normal_buffer_id) return NullId;
    auto maybe_texture_buffer_id = level->AddBuffer(std::move(texture_buffer));
    if (!maybe_texture_buffer_id) return NullId;
    auto maybe_index_buffer_id = level->AddBuffer(std::move(index_buffer));
    if (!maybe_index_buffer_id) return NullId;
    StaticMeshParameter parameter   = {};
    parameter.point_buffer_id       = maybe_point_buffer_id;
    parameter.normal_buffer_id      = maybe_normal_buffer_id;
    parameter.texture_buffer_id     = maybe_texture_buffer_id;
    parameter.index_buffer_id       = maybe_index_buffer_id;
    parameter.render_primitive_enum = proto::SceneStaticMesh::TRIANGLE;
    auto mesh                       = std::make_unique<StaticMesh>(level, parameter);
    mesh->SetName(fmt::format("CubeMesh.{}", count));
    return level->AddStaticMesh(std::move(mesh));
}

}  // End namespace frame::opengl.
