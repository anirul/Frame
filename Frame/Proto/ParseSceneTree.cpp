#include "ParseSceneTree.h"
#include "Frame/File/FileSystem.h"
#include "Frame/File/Obj.h"
#include "Frame/Proto/ParseUniform.h"
#include "Frame/NodeCamera.h"
#include "Frame/NodeLight.h"
#include "Frame/NodeMatrix.h"
#include "Frame/NodeStaticMesh.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/StaticMesh.h"

namespace frame::proto {

	std::shared_ptr<NodeMatrix> ParseSceneMatrix(
		const frame::proto::SceneMatrix& proto_scene_matrix)
	{
		auto scene_matrix = std::make_shared<NodeMatrix>(
			ParseUniform(proto_scene_matrix.matrix()));
		scene_matrix->SetName(proto_scene_matrix.name());
		scene_matrix->SetParentName(proto_scene_matrix.parent());
		return scene_matrix;
	}

	std::vector<std::shared_ptr<NodeStaticMesh>> ParseSceneStaticMesh(
		const frame::proto::SceneStaticMesh& proto_scene_static_mesh,
		LevelInterface* level)
	{
		std::vector<std::shared_ptr<NodeStaticMesh>> static_meshes = {};
		file::Obj obj(
			file::FindDirectory("Asset/Model/"),
			proto_scene_static_mesh.file_name());
		const auto meshes = obj.GetMeshes();
		frame::Logger& logger = Logger::GetInstance();
		const auto materials = obj.GetMaterials();
		logger->info(
			"Found in obj<{}> : {} materials.",
			proto_scene_static_mesh.file_name(),
			materials.size());
		for (const auto& material : materials)
		{
			throw std::runtime_error("This is not implemented.");
		}
		logger->info(
			"Found in obj<{}> : {} meshes.",
			proto_scene_static_mesh.file_name(),
			meshes.size());
		std::vector<float> points;
		std::vector<float> normals;
		std::vector<float> tex_coords;
		points.reserve(1024);
		normals.reserve(1024);
		tex_coords.reserve(1024);
		int mesh_counter = 0;
		for (const auto& mesh : meshes)
		{
			points.clear();
			normals.clear();
			tex_coords.clear();
			const auto& vertices = mesh.GetVertices();
			// TODO(anirul): could probably short this out!
			for (const auto& vertice : vertices)
			{
				points.push_back(vertice.point.x);
				points.push_back(vertice.point.y);
				points.push_back(vertice.point.z);
				normals.push_back(vertice.normal.x);
				normals.push_back(vertice.normal.y);
				normals.push_back(vertice.normal.z);
				tex_coords.push_back(vertice.tex_coord.x);
				tex_coords.push_back(vertice.tex_coord.y);
			}
			const auto& indices = mesh.GetIndices();
			// TODO(anirul): This should be unique ptr
			auto point_buffer = std::make_shared<opengl::Buffer>();
			auto normal_buffer = std::make_shared<opengl::Buffer>();
			auto texture_buffer = std::make_shared<opengl::Buffer>();
			auto index_buffer = std::make_shared<opengl::Buffer>(
				opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);

			// Position buffer initialization.
			point_buffer->Bind();
			point_buffer->Copy(
				points.size() * sizeof(float),
				points.data());
			point_buffer->UnBind();
			std::string mesh_name_point = fmt::format(
				"{}.{}.point",
				proto_scene_static_mesh.name(), 
				mesh_counter);
			const EntityId point_buffer_id =
				level->AddBuffer(mesh_name_point, point_buffer);

			// Normal buffer initialization.
			normal_buffer->Bind();
			normal_buffer->Copy(
				normals.size() * sizeof(float),
				normals.data());
			normal_buffer->UnBind();
			std::string mesh_name_normal = fmt::format(
				"{}.{}.normal",
				proto_scene_static_mesh.name(), 
				mesh_counter);
			const EntityId normal_buffer_id =
				level->AddBuffer(mesh_name_normal, normal_buffer);

			// Texture coordinates buffer initialization.
			texture_buffer->Bind();
			texture_buffer->Copy(
				tex_coords.size() * sizeof(float),
				tex_coords.data());
			texture_buffer->UnBind();
			std::string mesh_name_tex_coord = fmt::format(
				"{}.{}.tex_coord",
				proto_scene_static_mesh.name(),
				mesh_counter);
			const EntityId tex_coord_buffer_id =
				level->AddBuffer(mesh_name_tex_coord, texture_buffer);

			// Index buffer array.
			index_buffer->Bind();
			index_buffer->Copy(
				indices.size() * sizeof(std::int32_t),
				indices.data());
			index_buffer->UnBind();
			std::string mesh_name_index = fmt::format(
				"{}.{}.index",
				proto_scene_static_mesh.name(),
				mesh_counter);
			const EntityId index_buffer_id =
				level->AddBuffer(mesh_name_index, index_buffer);

			// This should also be a unique ptr.
			auto static_mesh = std::make_shared<opengl::StaticMesh>(
				level,
				point_buffer_id,
				normal_buffer_id,
				tex_coord_buffer_id,
				index_buffer_id);
			if (mesh.GetMaterialId() != -1)
			{
				throw std::runtime_error("No material implementation yet!");
			}
			std::string mesh_name = 
				fmt::format(
					"{}.{}", 
					proto_scene_static_mesh.name(), 
					mesh_counter);
			level->AddStaticMesh(mesh_name, static_mesh);
			mesh_counter++;
		}
		return static_meshes;
	}

	std::shared_ptr<NodeCamera> ParseSceneCamera(
		const frame::proto::SceneCamera& proto_scene_camera)
	{
		if (proto_scene_camera.fov_degrees() == 0.0)
			throw std::runtime_error("Need field of view degrees in camera.");
		auto scene_camera = std::make_shared<NodeCamera>(
			ParseUniform(proto_scene_camera.position()),
			ParseUniform(proto_scene_camera.target()),
			ParseUniform(proto_scene_camera.up()),
			proto_scene_camera.fov_degrees());
		scene_camera->SetName(proto_scene_camera.name());
		scene_camera->SetParentName(proto_scene_camera.parent());
		return scene_camera;
	}

	std::shared_ptr<NodeLight> ParseSceneLight(
		const frame::proto::SceneLight& proto_scene_light)
	{
		switch (proto_scene_light.light_type())
		{
			case proto::SceneLight::POINT:
			{
				return std::make_shared<frame::NodeLight>(
					NodeLightEnum::POINT, 
					ParseUniform(proto_scene_light.position()),
					ParseUniform(proto_scene_light.color()));
			}
			case proto::SceneLight::DIRECTIONAL:
			{
				return std::make_shared<frame::NodeLight>(
					NodeLightEnum::DIRECTIONAL,
					ParseUniform(proto_scene_light.direction()),
					ParseUniform(proto_scene_light.color()));
			}
			case proto::SceneLight::AMBIENT:
				[[fallthrough]];
			case proto::SceneLight::SPOT:
				[[fallthrough]];
			case proto::SceneLight::INVALID:
				[[fallthrough]];
			default:
				throw std::runtime_error(
					fmt::format(
						"Unknown scene light type {}", 
						proto_scene_light.light_type()));
		}
		return nullptr;
	}

	void ParseSceneTreeFile(
		const SceneTreeFile& proto_scene_tree_file,
		LevelInterface* level)
	{
		level->SetDefaultCameraName(
			proto_scene_tree_file.default_camera_name());
		level->SetDefaultRootSceneNodeName(
			proto_scene_tree_file.default_root_name());
		for (const auto& proto_matrix : proto_scene_tree_file.scene_matrices())
		{
			NodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<NodeInterface>(
					ParseSceneMatrix(proto_matrix));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
		for (const auto& proto_static_mesh :
			proto_scene_tree_file.scene_static_meshes())
		{
			for (const auto& ptr : 
				ParseSceneStaticMesh(proto_static_mesh, level))
			{
				level->AddSceneNode(ptr->GetName(), ptr);
			}
		}
		for (const auto& proto_camera : proto_scene_tree_file.scene_cameras())
		{
			NodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<NodeInterface>(
					ParseSceneCamera(proto_camera));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
		for (const auto& proto_light : proto_scene_tree_file.scene_lights())
		{
			NodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<NodeInterface>(
					ParseSceneLight(proto_light));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
	}

} // End namespace frame::proto.
