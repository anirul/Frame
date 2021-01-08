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
		throw std::runtime_error("implement me!");
		return nullptr;
	}

} // End namespace frame::proto.