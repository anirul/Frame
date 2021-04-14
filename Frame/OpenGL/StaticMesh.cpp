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
		index_size_ = level->GetBufferMap().at(index_buffer_id)->GetSize();

		// Create a new vertex array (to render the mesh).
		glGenVertexArrays(1, &vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindVertexArray(vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		level->GetBufferMap().at(point_buffer_id_)->Bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level->GetBufferMap().at(point_buffer_id_)->UnBind();
		level->GetBufferMap().at(normal_buffer_id_)->Bind();
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level->GetBufferMap().at(texture_buffer_id_)->Bind();
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level->GetBufferMap().at(texture_buffer_id_)->UnBind();

		// Enable vertex attrib array.
		glEnableVertexAttribArray(0);
		error_.Display(__FILE__, __LINE__ - 1);
		glEnableVertexAttribArray(1);
		error_.Display(__FILE__, __LINE__ - 1);
		glEnableVertexAttribArray(2);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	StaticMesh::~StaticMesh()
	{
		glDeleteVertexArrays(1, &vertex_array_object_);
	}

	void StaticMesh::Bind(const unsigned int slot /*= 0*/) const
	{
		if (locked_bind_) return;
		glBindVertexArray(vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void StaticMesh::UnBind() const
	{
		if (locked_bind_) return;
		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	EntityId CreateQuadStaticMesh(LevelInterface* level)
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
		std::shared_ptr<BufferInterface> point_buffer = 
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> normal_buffer = 
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> texture_buffer = 
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> index_buffer = 
			std::make_shared<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
		point_buffer->Bind();
		point_buffer->Copy(points.size() * sizeof(float), points.data());
		point_buffer->UnBind();
		normal_buffer->Bind();
		normal_buffer->Copy(normals.size() * sizeof(float), normals.data());
		normal_buffer->UnBind();
		texture_buffer->Bind();
		texture_buffer->Copy(textures.size() * sizeof(float), textures.data());
		texture_buffer->UnBind();
		index_buffer->Bind();
		index_buffer->Copy(indices.size() * sizeof(float), indices.data());
		index_buffer->UnBind();
		static int count = 0;
		auto mesh = std::make_shared<StaticMesh>(
			level, 
			level->AddBuffer(
				"QuadPoint" + std::to_string(count), 
				point_buffer),
			level->AddBuffer(
				"QuadNormal" + std::to_string(count), 
				normal_buffer),
			level->AddBuffer(
				"QuadTexture" + std::to_string(count), 
				texture_buffer),
			level->AddBuffer(
				"QuadIndex" + std::to_string(count), 
				index_buffer));
		auto id = level->AddStaticMesh(
			"QuadMesh" + std::to_string(count), 
			mesh);
		count++;
		return id;
	}

	EntityId CreateCubeStaticMesh(LevelInterface* level)
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
		std::shared_ptr<BufferInterface> point_buffer =
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> normal_buffer =
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> texture_buffer =
			std::make_shared<Buffer>();
		std::shared_ptr<BufferInterface> index_buffer =
			std::make_shared<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
		point_buffer->Bind();
		point_buffer->Copy(points.size() * sizeof(float), points.data());
		point_buffer->UnBind();
		normal_buffer->Bind();
		normal_buffer->Copy(normals.size() * sizeof(float), normals.data());
		normal_buffer->UnBind();
		texture_buffer->Bind();
		texture_buffer->Copy(textures.size() * sizeof(float), textures.data());
		texture_buffer->UnBind();
		index_buffer->Bind();
		index_buffer->Copy(indices.size() * sizeof(float), indices.data());
		index_buffer->UnBind();
		static int count = 0;
		auto mesh = std::make_shared<StaticMesh>(
			level,
			level->AddBuffer(
				"QuadPoint" + std::to_string(count),
				point_buffer),
			level->AddBuffer(
				"QuadNormal" + std::to_string(count),
				normal_buffer),
			level->AddBuffer(
				"QuadTexture" + std::to_string(count),
				texture_buffer),
			level->AddBuffer(
				"QuadIndex" + std::to_string(count),
				index_buffer));
		auto id = level->AddStaticMesh(
			"QuadMesh" + std::to_string(count),
			mesh);
		count++;
		return id;
	}

} // End namespace frame::opengl.
