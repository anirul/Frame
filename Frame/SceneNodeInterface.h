#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../Frame/StaticMeshInterface.h"

namespace frame {

	// Interface to visit the scene node.
	class SceneNodeInterface
	{
	public:
		// Redefinition for shortening.
		using Ptr = std::shared_ptr<SceneNodeInterface>;
		using PtrVec = std::vector<std::shared_ptr<SceneNodeInterface>>;

	public:
		// Get the local model of current node.
		virtual const glm::mat4 GetLocalModel(double dt) const = 0;
		// Get the local mesh of current node.
		virtual const std::shared_ptr<StaticMeshInterface> GetLocalMesh() const
		{
			return nullptr;
		}

	public:
		// Set a callback that will return a node according to a name. This
		// should be set at the time the scene interface is set to a scene tree.
		void SetCallback(std::function<Ptr(const std::string&)> func)
		{
			func_ = func;
		}
		// Return true if this is the root node (no parents).
		bool IsRoot() const { return GetParentName().empty(); }
		// Get the parent of a node.
		const std::string GetParentName() const { return parent_name_; }
		// Set the parent of a node.
		void SetParentName(const std::string& parent) { parent_name_ = parent; }
		// Getter for name.
		const std::string GetName() const { return name_; }
		// Setter for name.
		void SetName(const std::string& name) { name_ = name; }

	protected:
		std::function<Ptr(const std::string&)> func_ =
			[](const std::string) { return nullptr; };
		std::string parent_name_;
		std::string name_ = "";
	};

} // End namespace frame.