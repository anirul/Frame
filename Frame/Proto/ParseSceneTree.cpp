#include "ParseSceneTree.h"
#include "Frame/SceneCamera.h"
#include "Frame/SceneLight.h"
#include "Frame/SceneMatrix.h"
#include "Frame/SceneStaticMesh.h"

namespace frame::proto {

	std::shared_ptr<SceneMatrix> ParseSceneMatrixOpenGL(
		const frame::proto::SceneMatrix& proto_scene_matrix)
	{
		throw std::runtime_error("Implement me!");
		return nullptr;
	}

	std::shared_ptr<SceneStaticMesh> ParseSceneStaticMeshOpenGL(
		const frame::proto::SceneStaticMesh& proto_scene_static_mesh)
	{
		throw std::runtime_error("Implement me!");
		return nullptr;
	}

	std::shared_ptr<SceneCamera> ParseSceneCameraOpenGL(
		const frame::proto::SceneCamera& proto_scene_camera)
	{
		throw std::runtime_error("implement me!");
		return nullptr;
	}

	std::shared_ptr<SceneLight> ParseSceneLightOpenGL(
		const frame::proto::SceneLight& proto_scene_light)
	{
		throw std::runtime_error("Implement me!");
		return nullptr;
	}

	void ParseSceneTreeFileOpenGL(
		const SceneTreeFile& proto_scene_tree_file, 
		std::shared_ptr<LevelInterface> level)
	{
		level->SetDefaultCameraName(
			proto_scene_tree_file.default_camera_name());
		level->SetDefaultRootSceneNodeName(
			proto_scene_tree_file.default_root_name());
		for (const auto& proto_matrix : proto_scene_tree_file.scene_matrices())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneMatrixOpenGL(proto_matrix));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
		for (const auto& proto_static_mesh :
			proto_scene_tree_file.scene_static_meshes())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneStaticMeshOpenGL(proto_static_mesh));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
		for (const auto& proto_camera : proto_scene_tree_file.scene_cameras())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneCameraOpenGL(proto_camera));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
		for (const auto& proto_light : proto_scene_tree_file.scene_lights())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneLightOpenGL(proto_light));
			level->AddSceneNode(ptr->GetName(), ptr);
		}
	}

} // End namespace frame::proto.