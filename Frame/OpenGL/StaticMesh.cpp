#include "StaticMesh.h"
#include <numeric>
#include <iterator>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

namespace frame::opengl {

	StaticMesh::StaticMesh(
		LevelInterface* level,
		EntityId point_buffer_id,
		EntityId normal_buffer_id,
		EntityId texture_buffer_id,
		EntityId index_buffer_id,
		EntityId material_id)
	{
		// Get a local copy of the pointer.
		point_buffer_id_ = point_buffer_id;
		normal_buffer_id_ = normal_buffer_id;
		texture_buffer_id_ = texture_buffer_id;
		index_buffer_id_ = index_buffer_id;
		material_id_ = material_id;
		index_size_ = level->GetBufferFromId(index_buffer_id)->GetSize();

		// Create a new vertex array (to render the mesh).
		glGenVertexArrays(1, &vertex_array_object_);
		glBindVertexArray(vertex_array_object_);
		level->GetBufferFromId(point_buffer_id_)->Bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		level->GetBufferFromId(point_buffer_id_)->UnBind();
		level->GetBufferFromId(normal_buffer_id_)->Bind();
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		level->GetBufferFromId(texture_buffer_id_)->Bind();
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		level->GetBufferFromId(texture_buffer_id_)->UnBind();

		// Enable vertex attrib array.
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);
	}

	StaticMesh::~StaticMesh()
	{
		glDeleteVertexArrays(1, &vertex_array_object_);
	}

	void StaticMesh::Bind(const unsigned int slot /*= 0*/) const
	{
		if (locked_bind_) return;
		glBindVertexArray(vertex_array_object_);
	}

	void StaticMesh::UnBind() const
	{
		if (locked_bind_) return;
		glBindVertexArray(0);
	}

	std::optional<EntityId> CreateQuadStaticMesh(LevelInterface* level)
	{
		std::array<float, 12> points =
		{
			-1.f, 1.f, 0.f,
			1.f, 1.f, 0.f,
			-1.f, -1.f, 0.f,
			1.f, -1.f, 0.f,
		};
		std::array<float, 12> normals =
		{
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
		};
		std::array<float, 8> textures =
		{
			0, 1,
			1, 1,
			0, 0,
			1, 0,
		};
		std::array<std::int32_t, 6> indices =
		{
			0, 1, 2,
			1, 3, 2,
		};
		auto point_buffer = std::make_unique<Buffer>();
		auto normal_buffer = std::make_unique<Buffer>();
		auto texture_buffer = std::make_unique<Buffer>();
		auto index_buffer = std::make_unique<Buffer>(
			BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
		point_buffer->Copy(points.size() * sizeof(float), points.data());
		normal_buffer->Copy(normals.size() * sizeof(float), normals.data());
		texture_buffer->Copy(textures.size() * sizeof(float), textures.data());
		index_buffer->Copy(indices.size() * sizeof(float), indices.data());
		static int count = 0;
		count++;
		point_buffer->SetName(fmt::format("QuadPoint.{}", count));
		normal_buffer->SetName(fmt::format("QuadNormal.{}", count));
		texture_buffer->SetName(fmt::format("QuadTexture.{}", count));
		index_buffer->SetName(fmt::format("QuadIndex.{}", count));
		auto maybe_point_buffer_id = 
			level->AddBuffer(std::move(point_buffer));
		if (!maybe_point_buffer_id) return std::nullopt;
		auto maybe_normal_buffer_id = 
			level->AddBuffer(std::move(normal_buffer));
		if (!maybe_normal_buffer_id) return std::nullopt;
		auto maybe_texture_buffer_id = 
			level->AddBuffer(std::move(texture_buffer));
		if (!maybe_texture_buffer_id) return std::nullopt;
		auto maybe_index_buffer_id = 
			level->AddBuffer(std::move(index_buffer));
		if (!maybe_index_buffer_id) return std::nullopt;
		auto mesh = std::make_unique<StaticMesh>(
			level, 
			maybe_point_buffer_id.value(),
			maybe_normal_buffer_id.value(),
			maybe_texture_buffer_id.value(),
			maybe_index_buffer_id.value());
		mesh->SetName(fmt::format("QuadMesh.{}", count));
		auto maybe_id = level->AddStaticMesh(std::move(mesh));
		if (!maybe_id) return std::nullopt;
		return maybe_id.value();
	}

	std::optional<EntityId> CreateCubeStaticMesh(LevelInterface* level)
	{
		std::array<float, 18 * 6> points =
		{
			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,

			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,

			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,

			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
		};
		std::array<float, 18 * 6> normals =
		{
			.0f, .0f, -1.f,
			.0f, .0f, -1.f,
			.0f, .0f, -1.f,
			.0f, .0f, -1.f,
			.0f, .0f, -1.f,
			.0f, .0f, -1.f,

			.0f, .0f, 1.f,
			.0f, .0f, 1.f,
			.0f, .0f, 1.f,
			.0f, .0f, 1.f,
			.0f, .0f, 1.f,
			.0f, .0f, 1.f,

			-1.f, .0f, .0f,
			-1.f, .0f, .0f,
			-1.f, .0f, .0f,
			-1.f, .0f, .0f,
			-1.f, .0f, .0f,
			-1.f, .0f, .0f,
			
			1.f, .0f, .0f,
			1.f, .0f, .0f,
			1.f, .0f, .0f,
			1.f, .0f, .0f,
			1.f, .0f, .0f,
			1.f, .0f, .0f,
			 
			.0f, -1.f, -.0f,
			.0f, -1.f, -.0f,
			.0f, -1.f, -.0f,
			.0f, -1.f, -.0f,
			.0f, -1.f, -.0f,
			.0f, -1.f, -.0f,

			.0f, 1.f, 0.f,
			.0f, 1.f, 0.f,
			.0f, 1.f, 0.f,
			.0f, 1.f, 0.f,
			.0f, 1.f, 0.f,
			.0f, 1.f, 0.f,
		};
		std::array<float, 12 * 6> textures =
		{
			 0.0f, 0.0f,
			 1.0f, 0.0f,
			 1.0f, 1.0f,
			 1.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 0.0f,

			 0.0f, 0.0f,
			 1.0f, 0.0f,
			 1.0f, 1.0f,
			 1.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 0.0f,

			 1.0f, 0.0f,
			 1.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 0.0f,
			 1.0f, 0.0f,

			 1.0f, 0.0f,
			 1.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 1.0f,
			 0.0f, 0.0f,
			 1.0f, 0.0f,

			 0.0f, 1.0f,
			 1.0f, 1.0f,
			 1.0f, 0.0f,
			 1.0f, 0.0f,
			 0.0f, 0.0f,
			 0.0f, 1.0f,

			 0.0f, 1.0f,
			 1.0f, 1.0f,
			 1.0f, 0.0f,
			 1.0f, 0.0f,
			 0.0f, 0.0f,
			 0.0f, 1.0f
		};
		std::array<std::int32_t, 18 * 3> indices = {};
		std::iota(indices.begin(), indices.end(), 0);
		auto point_buffer =	std::make_unique<Buffer>();
		auto normal_buffer = std::make_unique<Buffer>();
		auto texture_buffer = std::make_unique<Buffer>();
		auto index_buffer = 
			std::make_unique<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
		point_buffer->Copy(points.size() * sizeof(float), points.data());
		normal_buffer->Copy(normals.size() * sizeof(float), normals.data());
		texture_buffer->Copy(textures.size() * sizeof(float), textures.data());
		index_buffer->Copy(
			indices.size() * sizeof(std::int32_t), 
			indices.data());
		static int count = 0;
		count++;
		point_buffer->SetName(fmt::format("CubePoint.{}", count));
		normal_buffer->SetName(fmt::format("CubeNormal.{}", count));
		texture_buffer->SetName(fmt::format("CubeTexture.{}", count));
		index_buffer->SetName(fmt::format("CubeIndex.{}", count));
		auto maybe_point_buffer_id =
			level->AddBuffer(std::move(point_buffer));
		if (!maybe_point_buffer_id) return std::nullopt;
		auto maybe_normal_buffer_id =
			level->AddBuffer(std::move(normal_buffer));
		if (!maybe_normal_buffer_id) return std::nullopt;
		auto maybe_texture_buffer_id =
			level->AddBuffer(std::move(texture_buffer));
		if (!maybe_texture_buffer_id) return std::nullopt;
		auto maybe_index_buffer_id =
			level->AddBuffer(std::move(index_buffer));
		if (!maybe_index_buffer_id) return std::nullopt;
		auto mesh = std::make_unique<StaticMesh>(
			level,
			maybe_point_buffer_id.value(),
			maybe_normal_buffer_id.value(),
			maybe_texture_buffer_id.value(),
			maybe_index_buffer_id.value());
		mesh->SetName(fmt::format("CubeMesh.{}", count));
		auto maybe_id = level->AddStaticMesh(std::move(mesh));
		if (!maybe_id) return std::nullopt;
		return maybe_id.value();
	}

} // End namespace frame::opengl.
