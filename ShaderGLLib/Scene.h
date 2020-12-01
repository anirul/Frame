#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include "../ShaderGLLib/Mesh.h"
#include "../ShaderGLLib/Material.h"

namespace sgl {

	// Interface to visit the scene tree.
	class SceneInterface
	{
	public:
		// Redefinition for shortening.
		using Ptr = std::shared_ptr<SceneInterface>;
		using PtrVec = std::vector<std::shared_ptr<SceneInterface>>;
		// Get the local model of current node.
		virtual const glm::mat4 GetLocalModel(const double dt) const = 0;
		// Get the local mesh of current node.
		virtual const std::shared_ptr<Mesh> GetLocalMesh() const = 0;

	public:
		// Return true if this is the root node (no parents).
		bool IsRoot() const { return !GetParent(); }
		// Get the parent of a node.
		const Ptr GetParent() const { return parent_; }
		// Set the parent of a node.
		void SetParent(Ptr parent) { parent_ = parent; }

	protected:
		Ptr parent_;
	};

	class SceneMatrix : public SceneInterface
	{
	public:
		SceneMatrix(const glm::mat4& matrix) : matrix_(matrix) {}
		SceneMatrix(const std::function<glm::mat4(const double)> func) : 
			func_(func) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override;

	private:
		glm::mat4 matrix_ = {};
		std::function<glm::mat4(const double)> func_ = 
			[this](const double) { return matrix_; };
	};

	class SceneMesh : public SceneInterface
	{
	public:
		SceneMesh(std::shared_ptr<sgl::Mesh> mesh) : mesh_(mesh) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override; 

	private:
		std::shared_ptr<sgl::Mesh> mesh_ = nullptr;
	};

	class SceneTree
	{
	public:
		// Create a default empty scene tree. 
		SceneTree() = default;
		// Create a scene tree from a proto file.
		SceneTree(const frame::proto::Scene& proto);

	public:
		const SceneInterface::PtrVec GetSceneVector() const { return scene_; }

	public:
		// Add a node to the scene tree.
		void AddNode(
			const SceneInterface::Ptr node, 
			const SceneInterface::Ptr parent = nullptr);
		// Get the root of the scene tree.
		const SceneInterface::Ptr GetRoot() const;

	protected:
		// Contain the scene.
		SceneInterface::PtrVec scene_;
	};

	SceneTree LoadSceneFromObjStream(
		std::istream& is, 
		const std::shared_ptr<ProgramInterface> program,
		const std::string& name);

} // End namespace sgl.
