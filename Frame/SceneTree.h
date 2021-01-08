#pragma once

#include "Frame/SceneTreeInterface.h"

namespace frame {

	class SceneTree : public SceneTreeInterface
	{
	public:
		// Create a default empty scene tree. 
		SceneTree() = default;

	public:
		// Return a map of scene names and scene components.
		const std::map<std::string, SceneNodeInterface::Ptr>
			GetSceneMap() const override;
		// Return the element at name position (or nullptr).
		const SceneNodeInterface::Ptr GetSceneByName(
			const std::string& name) const override;
		// Add a node to the scene tree. This will also add the callback to the
		// node to the GetSceneByName function.
		void AddNode(const SceneNodeInterface::Ptr node) override;
		// Get the root of the scene tree.
		const SceneNodeInterface::Ptr GetRoot() const override;
		// Set the default camera node.
		void SetDefaultCamera(const std::string& camera_name) override;
		// Get a pointer to the default camera.
		std::shared_ptr<CameraInterface> GetDefaultCamera() override;
		// Same but const version.
		const std::shared_ptr<CameraInterface> GetDefaultCamera(
			) const override;

	private:
		// Contain the scene.
		std::map<std::string, SceneNodeInterface::Ptr> scene_map_;
		// Name of the scene.
		std::string name_;
		// Name of the root node.
		std::string root_node_name_;
		// Store the default camera node.
		std::string camera_name_;
	};

} // End namespace frame.
