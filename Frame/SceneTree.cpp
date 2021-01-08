#include "SceneTree.h"
#include <stdexcept>
#include "Frame/SceneCamera.h"

namespace frame {

	const std::map<std::string, SceneNodeInterface::Ptr>
		SceneTree::GetSceneMap() const
	{
		return scene_map_;
	}

	const SceneNodeInterface::Ptr SceneTree::GetSceneByName(
		const std::string& name) const
	{
		return scene_map_.at(name);
	}

	void SceneTree::AddNode(const SceneNodeInterface::Ptr node)
	{
		node->SetCallback([this](const std::string& name)
			{
				return this->GetSceneByName(name);
			});
		if (scene_map_.find(node->GetName()) != scene_map_.end())
		{
			throw std::runtime_error(
				"node: [" + node->GetName() + "] is already in!");
		}
		scene_map_.insert({ node->GetName(), node });
	}

	const SceneNodeInterface::Ptr SceneTree::GetRoot() const
	{
		SceneNodeInterface::Ptr ret = nullptr;
		for (const auto& pair : scene_map_)
		{
			const auto& scene = pair.second;
			if (scene->GetParentName().empty())
			{
				if (ret) throw std::runtime_error("More than one root?");
				ret = scene;
			}
		}
		return ret;
	}

	void SceneTree::SetDefaultCamera(const std::string& camera_name)
	{
		camera_name_ = camera_name;
	}

	std::shared_ptr<CameraInterface> SceneTree::GetDefaultCamera()
	{
		std::shared_ptr<SceneNodeInterface> ptr = GetSceneByName(camera_name_);
		std::shared_ptr<SceneCamera> scene_camera = 
			std::dynamic_pointer_cast<SceneCamera>(ptr);
		if (scene_camera)
		{
			return scene_camera->GetCamera();
		}
		throw std::runtime_error("no camera in the scene.");
	}

	const std::shared_ptr<CameraInterface> SceneTree::GetDefaultCamera() const
	{
		const std::shared_ptr<SceneNodeInterface> ptr = GetSceneByName(camera_name_);
		const std::shared_ptr<SceneCamera> scene_camera =
			std::dynamic_pointer_cast<SceneCamera>(ptr);
		if (scene_camera)
		{
			return scene_camera->GetCamera();
		}
		throw std::runtime_error("no camera in the scene.");
	}

} // End namespace frame.
