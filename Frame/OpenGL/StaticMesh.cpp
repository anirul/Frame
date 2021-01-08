#include "StaticMesh.h"
#include <iterator>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

namespace frame::opengl {

	StaticMesh::StaticMesh(
		const std::shared_ptr<LevelInterface> level,
		std::uint64_t point_buffer_id,
		std::uint64_t normal_buffer_id,
		std::uint64_t texture_buffer_id,
		std::uint64_t index_buffer_id)
	{
		// Get a local copy of the pointer.
		level_ = level;

		// Create a new vertex array (to render the mesh).
		glGenVertexArrays(1, &vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindVertexArray(vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		level_->GetBufferMap().at(point_buffer_id_)->Bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level_->GetBufferMap().at(point_buffer_id_)->UnBind();
		level_->GetBufferMap().at(normal_buffer_id_)->Bind();
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level_->GetBufferMap().at(texture_buffer_id_)->Bind();
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		level_->GetBufferMap().at(texture_buffer_id_)->UnBind();

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

	std::shared_ptr<frame::StaticMeshInterface> CreateQuadStaticMesh(
		std::shared_ptr<LevelInterface> level)
	{
		throw std::runtime_error("Not implemented!");
	}

	std::shared_ptr<frame::StaticMeshInterface> CreateCubeStaticMesh(
		std::shared_ptr<LevelInterface> level)
	{
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::opengl.
