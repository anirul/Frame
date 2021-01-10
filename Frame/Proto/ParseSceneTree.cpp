#include "Frame/Proto/ParseSceneTree.h"
#include "Frame/SceneCamera.h"
#include "Frame/SceneLight.h"
#include "Frame/SceneMatrix.h"
#include "Frame/SceneTree.h"
#include "Frame/SceneStaticMesh.h"

namespace frame::proto {

	std::shared_ptr<SceneMatrix> ParseSceneMatrixOpenGL(
		const frame::proto::SceneMatrix& proto_scene_matrix)
	{
		throw std::runtime_error("implement me!");
		return nullptr;
	}

	std::shared_ptr<SceneStaticMesh> ParseSceneStaticMeshOpenGL(
		const frame::proto::SceneStaticMesh& proto_scene_static_mesh)
	{
		throw std::runtime_error("implement me!");
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
		throw std::runtime_error("implement me!");
		return nullptr;
	}

	std::shared_ptr<SceneTreeInterface> ParseSceneTreeOpenGL(
		const frame::proto::SceneTree& proto_scene_tree)
	{
		auto scene_tree = std::make_shared<frame::SceneTree>(
			proto_scene_tree.name());
		scene_tree->SetDefaultCameraName(
			proto_scene_tree.default_camera_name());
		scene_tree->SetDefaultRootName(proto_scene_tree.default_root_name());
		for (const auto& proto_matrix : proto_scene_tree.scene_matrices())
		{
			SceneNodeInterface::Ptr ptr = 
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneMatrixOpenGL(proto_matrix));
			scene_tree->AddNode(ptr);
		}
		for (const auto& proto_static_mesh : 
			proto_scene_tree.scene_static_meshes())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneStaticMeshOpenGL(proto_static_mesh));
			scene_tree->AddNode(ptr);
		}
		for (const auto& proto_camera : proto_scene_tree.scene_cameras())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneCameraOpenGL(proto_camera));
			scene_tree->AddNode(ptr);
		}
		for (const auto& proto_light : proto_scene_tree.scene_lights())
		{
			SceneNodeInterface::Ptr ptr =
				std::dynamic_pointer_cast<SceneNodeInterface>(
					ParseSceneLightOpenGL(proto_light));
			scene_tree->AddNode(ptr);
		}
		return scene_tree;
	}

} // End namespace frame::proto.