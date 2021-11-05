#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Frame/NameInterface.h"
#include "Frame/StaticMeshInterface.h"

namespace frame {

	// Interface to visit the scene node.
	struct NodeInterface : public NameInterface
	{
	public:
		// Get the local model of current node.
		virtual glm::mat4 GetLocalModel(double dt) const = 0;
		// Get the local mesh of current node.
		virtual EntityId GetLocalMesh() const { return 0; }

	public:
		// Constructor for NodeInterface, it take a function as a parameter
		// this function return the ID from a string (it will need a level).
		NodeInterface(std::function<NodeInterface*(const std::string&)> func)
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
		std::string GetName() const override { return name_; }
		// Setter for name.
		void SetName(const std::string& name) override { name_ = name; }

	protected:
		std::function<NodeInterface*(const std::string&)> func_ =
			[](const std::string) { return nullptr; };
		std::string parent_name_ = "";
		std::string name_ = "";
	};

} // End namespace frame.