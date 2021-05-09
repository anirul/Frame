#include "Frame/OpenGL/File/LoadStaticMesh.h"
#include <stdexcept>
#include "Frame/File/FileSystem.h"
#include "Frame/Logger.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/OpenGL/StaticMesh.h"

namespace frame::opengl::file {

	namespace {

		template <typename T>
		EntityId CreateBufferInLevel(
			LevelInterface* level,
			const std::vector<T>& vec, 
			const std::string& desc,
			const BufferTypeEnum buffer_type = 
				BufferTypeEnum::ARRAY_BUFFER,
			const BufferUsageEnum buffer_usage =
				BufferUsageEnum::STATIC_DRAW)
		{
			std::shared_ptr<BufferInterface> buffer = 
				std::make_shared<Buffer>(buffer_type, buffer_usage);
			// Buffer initialization.
			buffer->Bind();
			buffer->Copy(
				vec.size() * sizeof(T),
				vec.data());
			buffer->UnBind();
			return level->AddBuffer(desc, buffer);
		}

		std::shared_ptr<TextureInterface> LoadTextureFromString(
			const std::string& str,
			const proto::PixelElementSize pixel_element_size,
			const proto::PixelStructure pixel_structure)
		{
			return opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/" + str),
				pixel_element_size,
				pixel_structure);
		}

		EntityId LoadMaterialFromObj(
			LevelInterface* level,
			const frame::file::ObjMaterial& material_obj)
		{
			// Load textures.
			std::shared_ptr<TextureInterface> color = 
				(material_obj.ambient_str.empty()) ?
					LoadTextureFromVec4(material_obj.ambient_vec4) :
					LoadTextureFromString(
						material_obj.ambient_str,
						proto::PixelElementSize_BYTE(),
						proto::PixelStructure_RGB());
			std::shared_ptr<TextureInterface> normal =
				(material_obj.normal_str.empty()) ?
					LoadTextureFromVec4(glm::vec4(0.f, 0.f, 0.f, 1.f)) :
					LoadTextureFromString(material_obj.normal_str,
						proto::PixelElementSize_BYTE(),
						proto::PixelStructure_RGB());
			std::shared_ptr<TextureInterface> roughness =
				(material_obj.roughness_str.empty()) ?
					LoadTextureFromFloat(material_obj.roughness_val) :
					LoadTextureFromString(material_obj.roughness_str,
						proto::PixelElementSize_BYTE(),
						proto::PixelStructure_GREY());
			std::shared_ptr<TextureInterface> metallic = 
				(material_obj.metallic_str.empty()) ?
					LoadTextureFromFloat(material_obj.metallic_val) :
					LoadTextureFromString(material_obj.metallic_str,
						proto::PixelElementSize_BYTE(),
						proto::PixelStructure_GREY());
			// Create names for textures.
			auto color_name = fmt::format("{}.Color", material_obj.name);
			auto normal_name = fmt::format("{}.Normal", material_obj.name);
			auto roughness_name = fmt::format("{}.Roughness", material_obj.name);
			auto metallic_name = fmt::format("{}.Metallic", material_obj.name);
			// Add texture to the level.
			auto color_id = level->AddTexture(color_name, color);
			auto normal_id = level->AddTexture(normal_name,	normal);
			auto roughness_id = level->AddTexture(roughness_name, roughness);
			auto metallic_id = level->AddTexture(metallic_name, metallic);
			// Create the material.
			std::shared_ptr<MaterialInterface> material =
				std::make_shared<opengl::Material>();
			// Add texture to the material.
			material->AddTextureId(color_id, color_name);
			material->AddTextureId(normal_id, normal_name);
			material->AddTextureId(roughness_id, roughness_name);
			material->AddTextureId(metallic_id, metallic_name);
			// Finally add the material to the level.
			return level->AddMaterial(material_obj.name, material);
		}

		EntityId LoadStaticMeshFromObj(
			LevelInterface* level,
			const frame::file::ObjMesh& mesh_obj,
			const std::string& name,
			const std::vector<EntityId> material_ids,
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
			const EntityId point_buffer_id =
				CreateBufferInLevel(
					level, 
					points, 
					fmt::format("{}.{}.point",	name, counter));

			// Normal buffer initialization.
			const EntityId normal_buffer_id =
				CreateBufferInLevel(
					level,
					normals,
					fmt::format("{}.{}.normal", name, counter));

			// Texture coordinates buffer initialization.
			const EntityId tex_coord_buffer_id =
				CreateBufferInLevel(
					level,
					textures,
					fmt::format("{}.{}.texture", name, counter));
		
			// Index buffer array.
			const EntityId index_buffer_id =
				CreateBufferInLevel(
					level,
					indices,
					fmt::format("{}.{}.index", name, counter),
					opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);

			// This should also be a unique ptr.
			auto static_mesh = std::make_shared<opengl::StaticMesh>(
				level,
				point_buffer_id,
				normal_buffer_id,
				tex_coord_buffer_id,
				index_buffer_id);
			if (mesh_obj.GetMaterialId() != -1)
			{
				static_mesh->SetMaterialId(
					material_ids.at(mesh_obj.GetMaterialId()));
			}
			std::string mesh_name =
				fmt::format(
					"{}.{}",
					name,
					counter);
			return level->AddStaticMesh(mesh_name, static_mesh);
		}

	} // End namespace.

	std::vector<std::shared_ptr<NodeStaticMesh>> LoadStaticMeshesFromFile(
		LevelInterface* level, 
		const std::string& file,
		const std::string& name)
	{
		std::vector<std::shared_ptr<NodeStaticMesh>> static_mesh_vec;
		frame::file::Obj obj(frame::file::FindFile(file));
		const auto meshes = obj.GetMeshes();
		Logger& logger = Logger::GetInstance();
		const auto materials = obj.GetMaterials();
		logger->info(
			"Found in obj<{}> : {} materials.",
			file,
			materials.size());
		std::vector<EntityId> material_ids;
		for (const auto& material : materials)
		{
			const EntityId id = LoadMaterialFromObj(
				level,
				material);
			material_ids.push_back(id);
		}
		logger->info("Found in obj<{}> : {} meshes.", file, meshes.size());
		int mesh_counter = 0;
		for (const auto& mesh : meshes)
		{
			EntityId static_mesh_id = 
				LoadStaticMeshFromObj(
					level, 
					mesh, 
					name, 
					material_ids,
					mesh_counter);
			static_mesh_vec.push_back(
				std::make_shared<NodeStaticMesh>(
					static_mesh_id,
					level->GetStaticMeshMap().at(
						static_mesh_id)->GetMaterialId()));
			mesh_counter++;
		}
		return static_mesh_vec;
	}

} // End namespace frame::file.
