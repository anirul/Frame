#include "ParseSceneTree.h"

#include "Frame/File/FileSystem.h"
#include "Frame/File/Obj.h"
#include "Frame/Proto/ParseUniform.h"
#include "Frame/NodeCamera.h"
#include "Frame/NodeLight.h"
#include "Frame/NodeMatrix.h"
#include "Frame/NodeStaticMesh.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/File/LoadStaticMesh.h"
#include "Frame/OpenGL/StaticMesh.h"

namespace frame::proto {

	namespace {

		std::function<NodeInterface*(const std::string& name)> 
		GetFunctor(LevelInterface* level)
		{
			return [level](const std::string& name)->NodeInterface*
			{
				if (level) {
					auto maybe_id = level->GetIdFromName(name);
					if (!maybe_id) 
					{
						throw std::runtime_error(
							fmt::format("No id from name: {}", name));
					}
					EntityId id = maybe_id.value();
					return level->GetSceneNodeFromId(id);
				}
				return nullptr;
			};
		}

		[[nodiscard]] bool ParseSceneMatrix(
			LevelInterface* level,
			const SceneMatrix& proto_scene_matrix)
		{
			auto scene_matrix = 
				std::make_unique<NodeMatrix>(
					GetFunctor(level),
					ParseUniform(proto_scene_matrix.matrix()));
			scene_matrix->SetName(proto_scene_matrix.name());
			scene_matrix->SetParentName(proto_scene_matrix.parent());
			for (auto flag : proto_scene_matrix.clean_buffer().values())
			{
				scene_matrix->AddClearFlags(flag);
			}
			auto maybe_scene_id = level->AddSceneNode(std::move(scene_matrix));
			return static_cast<bool>(maybe_scene_id);
		}

		[[nodiscard]] bool ParseSceneStaticMesh(
			LevelInterface* level,
			const SceneStaticMesh& proto_scene_static_mesh)
		{
			if (proto_scene_static_mesh.file_name().empty())
			{
				if (proto_scene_static_mesh.mesh_enum() == 
					SceneStaticMesh::INVALID)
				{
					throw std::runtime_error(
						"Didn't find any mesh file name or any enum.");
				}
				// In this case there is only one material per mesh.
				EntityId mesh_id = 0;
				switch (proto_scene_static_mesh.mesh_enum())
				{
					case SceneStaticMesh::CUBE:
					{
						auto maybe_mesh_id = 
							level->GetDefaultStaticMeshCubeId();
						if (!maybe_mesh_id) return false;
						mesh_id = maybe_mesh_id.value();
						break;
					}
					case SceneStaticMesh::QUAD:
					{
						auto maybe_mesh_id =
							level->GetDefaultStaticMeshQuadId();
						if (!maybe_mesh_id) return false;
						mesh_id = maybe_mesh_id.value();
						break;
					}
					default:
					{
						throw std::runtime_error(
							fmt::format(
								"unknown mesh enum value: {}", 
								proto_scene_static_mesh.mesh_enum()));
					}
				}
				auto maybe_material_id = level->GetIdFromName(
					proto_scene_static_mesh.material_name());
				if (!maybe_material_id) return false;
				const EntityId material_id = maybe_material_id.value();
				std::unique_ptr<NodeInterface> node_interface = 
					std::make_unique<NodeStaticMesh>(
						GetFunctor(level), 
						mesh_id, 
						material_id);
				node_interface->SetName(proto_scene_static_mesh.name());
				node_interface->SetParentName(proto_scene_static_mesh.parent());
				auto maybe_scene_id = 
					level->AddSceneNode(std::move(node_interface));
				return static_cast<bool>(maybe_scene_id);
			}
			else
			{
				// TODO(anirul): this should be OpenGL agnostic.
				auto maybe_vec_node_mesh_id = 
					opengl::file::LoadStaticMeshesFromFile(
						level,
						"Asset/Model/" + proto_scene_static_mesh.file_name(),
						proto_scene_static_mesh.name(),
						proto_scene_static_mesh.material_name(),
						proto_scene_static_mesh.skip_file_material());
				if (!maybe_vec_node_mesh_id) return false;
				auto& vec_node_mesh_id = maybe_vec_node_mesh_id.value();
				int i = 0;
				for (const auto node_mesh_id : vec_node_mesh_id)
				{
					auto node = level->GetSceneNodeFromId(node_mesh_id);
					auto mesh = 
						level->GetStaticMeshFromId(node->GetLocalMesh());
					auto str = fmt::format(
						"{}.{}", 
						proto_scene_static_mesh.name(), 
						i);
					mesh->SetName(str);
					node->SetParentName(proto_scene_static_mesh.parent());
					++i;
				}
			}
			return true;
		}

		[[nodiscard]] bool ParseSceneCamera(
			LevelInterface* level,
			const frame::proto::SceneCamera& proto_scene_camera)
		{
			if (proto_scene_camera.fov_degrees() == 0.0)
			{
				throw std::runtime_error(
					"Need field of view degrees in camera.");
			}
			std::unique_ptr<NodeInterface> scene_camera =
				std::make_unique<NodeCamera>(
					GetFunctor(level),
					ParseUniform(proto_scene_camera.position()),
					ParseUniform(proto_scene_camera.target()),
					ParseUniform(proto_scene_camera.up()),
					proto_scene_camera.fov_degrees(),
					proto_scene_camera.aspect_ratio(),
					proto_scene_camera.near_clip(),
					proto_scene_camera.far_clip());
			scene_camera->SetName(proto_scene_camera.name());
			scene_camera->SetParentName(proto_scene_camera.parent());
			auto maybe_scene_id = level->AddSceneNode(std::move(scene_camera));
			return static_cast<bool>(maybe_scene_id);
		}

		[[nodiscard]] bool ParseSceneLight(
			LevelInterface* level,
			const proto::SceneLight& proto_scene_light)
		{
			switch (proto_scene_light.light_type())
			{
				case proto::SceneLight::POINT:
				{
					std::unique_ptr<NodeInterface> node_light = 
						std::make_unique<frame::NodeLight>(
							GetFunctor(level),
							NodeLightEnum::POINT, 
							ParseUniform(proto_scene_light.position()),
							ParseUniform(proto_scene_light.color()));
					auto maybe_node_id = 
						level->AddSceneNode(std::move(node_light));
					return static_cast<bool>(maybe_node_id);
				}
				case proto::SceneLight::DIRECTIONAL:
				{
					std::unique_ptr<NodeInterface> node_light = 
						std::make_unique<frame::NodeLight>(
							GetFunctor(level),
							NodeLightEnum::DIRECTIONAL,
							ParseUniform(proto_scene_light.direction()),
							ParseUniform(proto_scene_light.color()));
					auto maybe_node_id = 
						level->AddSceneNode(std::move(node_light));
					return static_cast<bool>(maybe_node_id);
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
			return false;
		}

	} // End namespace.

	[[nodiscard]] bool ParseSceneTreeFile(
		const SceneTree& proto_scene_tree,
		LevelInterface* level)
	{
		level->SetDefaultCameraName(
			proto_scene_tree.default_camera_name());
		level->SetDefaultRootSceneNodeName(
			proto_scene_tree.default_root_name());
		for (const auto& proto_matrix : proto_scene_tree.scene_matrices())
		{
			if (!ParseSceneMatrix(level, proto_matrix))
				return false;
		}
		for (const auto& proto_static_mesh :
			proto_scene_tree.scene_static_meshes())
		{
			if (!ParseSceneStaticMesh(level, proto_static_mesh))
				return false;
		}
		for (const auto& proto_camera : proto_scene_tree.scene_cameras())
		{
			if (!ParseSceneCamera(level, proto_camera))
				return false;
		}
		for (const auto& proto_light : proto_scene_tree.scene_lights())
		{
			if (!ParseSceneLight(level, proto_light))
				return false;
		}
		return true;
	}

} // End namespace frame::proto.
