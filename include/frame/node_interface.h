#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "frame/name_interface.h"
#include "frame/static_mesh_interface.h"

namespace frame {

/**
 * @class NodeInterface
 * @brief Interface to visit the scene node.
 */
struct NodeInterface : public NameInterface {
 public:
  /**
   * @brief Constructor for NodeInterface, it take a function as a parameter
   * this function return the ID from a string (it will need a level passed in
   * the capture list).
   * @param func: This function return the ID from a string (it will need a
   * level passed in the capture list).
   */
  NodeInterface(std::function<NodeInterface*(const std::string&)> func) {
    func_ = func;
  }
  //! @brief Virtual destructor.
  virtual ~NodeInterface() = default;

 public:
  /**
   * @brief Compute the local model of current node.
   * @return A mat4 representing the local model matrix.
   */
  virtual glm::mat4 GetLocalModel(double dt) const = 0;

 public:
  /**
   * @brief Get the local mesh of current node.
   * @return Id of a local mesh (if present).
   */
  virtual EntityId GetLocalMesh() const { return 0; }
  /**
   * @brief Check if this is the root node (no parents).
   * @return True if this is the root node (no parents).
   */
  bool IsRoot() const { return GetParentName().empty(); }
  /**
   * @brief Get the name of the parent node.
   * @return String representation of the name of parent node.
   */
  const std::string GetParentName() const { return parent_name_; }
  /**
   * @brief Set the parent node name.
   * @param parent: Set the name of the parent name node.
   */
  void SetParentName(const std::string& parent) { parent_name_ = parent; }
  /**
   * @brief Get name from the name interface.
   * @return The name of the object.
   */
  std::string GetName() const override { return name_; }
  /**
   * @brief Set name from the name interface.
   * @param name: New name to be set.
   */
  void SetName(const std::string& name) override { name_ = name; }

 protected:
  std::function<NodeInterface*(const std::string&)> func_ =
      [](const std::string&) -> NodeInterface* { return nullptr; };
  std::string parent_name_;
  std::string name_;
};

}  // End namespace frame.
