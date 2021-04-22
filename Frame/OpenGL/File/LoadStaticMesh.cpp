#include "Frame/OpenGL/File/LoadStaticMesh.h"
#include <stdexcept>
#include "Frame/File/FileSystem.h"
#include "Frame/Logger.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/StaticMesh.h"

namespace {

	template <typename T>
	frame::EntityId CreateBufferInLevel(
		frame::LevelInterface* level,
		const std::vector<T>& vec, 
		const std::string& desc,
		const frame::opengl::BufferTypeEnum buffer_type = 
			frame::opengl::BufferTypeEnum::ARRAY_BUFFER,
		const frame::opengl::BufferUsageEnum buffer_usage =
			frame::opengl::BufferUsageEnum::STATIC_DRAW)
	{
		std::shared_ptr<frame::BufferInterface> buffer = 
			std::make_shared<frame::opengl::Buffer>(buffer_type, buffer_usage);
		// Buffer initialization.
		buffer->Bind();
		buffer->Copy(
			vec.size() * sizeof(T),
			vec.data());
		buffer->UnBind();
		return level->AddBuffer(desc, buffer);
	}

	frame::EntityId LoadStaticMeshFromObj(
		frame::LevelInterface* level,
		const frame::file::ObjMesh& mesh_obj,
		const std::string& name,
		int counter)
	{
		std::vector<float> points;
		std::vector<float> normals;
		std::vector<float> textures;
		const auto& vertices = mesh_obj.GetVertices();
		// TODO(anirul): could probably short this out!
		for (const auto& vertice : vertices)
		{
			points.push_back(vertice.point.x);
			points.push_back(vertice.point.y);
			points.push_back(vertice.point.z);
			normals.push_back(vertice.normal.x);
			normals.push_back(vertice.normal.y);
			normals.push_back(vertice.normal.z);
			textures.push_back(vertice.tex_coord.x);
			textures.push_back(vertice.tex_coord.y);
		}
		const auto& indices = mesh_obj.GetIndices();

		// Point buffer initialization.
		const frame::EntityId point_buffer_id =
			CreateBufferInLevel(
				level, 
				points, 
				fmt::format("{}.{}.point",	name, counter));

		// Normal buffer initialization.
		const frame::EntityId normal_buffer_id =
			CreateBufferInLevel(
				level,
				normals,
				fmt::format("{}.{}.normal", name, counter));

		// Texture coordinates buffer initialization.
		const frame::EntityId tex_coord_buffer_id =
			CreateBufferInLevel(
				level,
				textures,
				fmt::format("{}.{}.texture", name, counter));
		
		// Index buffer array.
		const frame::EntityId index_buffer_id =
			CreateBufferInLevel(
				level,
				indices,
				fmt::format("{}.{}.index", name, counter),
				frame::opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);

		// This should also be a unique ptr.
		auto static_mesh = std::make_shared<frame::opengl::StaticMesh>(
			level,
			point_buffer_id,
			normal_buffer_id,
			tex_coord_buffer_id,
			index_buffer_id);
		if (mesh_obj.GetMaterialId() != -1)
		{
			throw std::runtime_error("No material implementation yet!");
		}
		std::string mesh_name =
			fmt::format(
				"{}.{}",
				name,
				counter);
		return level->AddStaticMesh(mesh_name, static_mesh);
	}

} // End namespace.

namespace frame::opengl::file {

	std::vector<std::shared_ptr<NodeStaticMesh>> LoadStaticMeshesFromFile(
		LevelInterface* level, 
		const std::string& file,
		const std::string& name)
	{
		std::vector<std::shared_ptr<NodeStaticMesh>> static_mesh_vec;
		frame::file::Obj obj(frame::file::FindFile(file));
		const auto meshes = obj.GetMeshes();
		frame::Logger& logger = Logger::GetInstance();
		const auto materials = obj.GetMaterials();
		logger->info(
			"Found in obj<{}> : {} materials.",
			file,
			materials.size());
		for (const auto& material : materials)
		{
			throw std::runtime_error("This is not implemented.");
		}
		logger->info("Found in obj<{}> : {} meshes.", file, meshes.size());
		int mesh_counter = 0;
		for (const auto& mesh : meshes)
		{
			frame::EntityId static_mesh_id = 
				LoadStaticMeshFromObj(level, mesh, name, mesh_counter);
			static_mesh_vec.push_back(
				std::make_shared<NodeStaticMesh>(
					level->GetStaticMeshMap().at(static_mesh_id)));
			mesh_counter++;
		}
		return static_mesh_vec;
	}

} // End namespace frame::file.
