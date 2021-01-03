#pragma once

#include <memory>
#include <map>
#include "../Frame/SceneNodeInterface.h"
#include "../Frame/CameraInterface.h"

namespace frame {

	struct SceneTreeInterface
	{
		// Return a map of scene names and scene components.
		// CHECKME(anirul): Shouldn't this be an unordered map?
		virtual const std::map<std::string, SceneNodeInterface::Ptr> 
			GetSceneMap() const = 0;
		// Return the element at name position (or nullptr).
		virtual const SceneNodeInterface::Ptr GetSceneByName(
			const std::string& name) const = 0;
		// Add a node to the scene tree. This will also add the callback to the
		// node to the GetSceneByName function.
		virtual void AddNode(const SceneNodeInterface::Ptr node) = 0;
		// Get the root of the scene tree.
		virtual const SceneNodeInterface::Ptr GetRoot() const = 0;
		// Set the default camera node.
		virtual void SetDefaultCamera(const std::string& camera_name) = 0;
		// Get a pointer to the default camera.
		virtual CameraInterface& GetDefaultCamera() = 0;
		// Same but const.
		virtual const CameraInterface& GetDefaultCamera() const = 0;
	};

} // End namespace frame.
