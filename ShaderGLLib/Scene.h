#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include "../ShaderGLLib/Mesh.h"

namespace sgl {

	struct Scene 
	{
		virtual void SetParent(const std::shared_ptr<Scene>& parent) = 0;
		virtual const std::shared_ptr<Scene>& GetParent() const = 0;
		virtual const glm::mat4 GetLocalModel(const double dt) const = 0;
		virtual const std::shared_ptr<Mesh> GetLocalMesh() const = 0;
		virtual bool IsLeaf() const = 0;
		virtual bool IsRoot() const = 0;
	};

	class SceneMatrix : public Scene 
	{
	public:
		SceneMatrix(const glm::mat4& matrix) : matrix_(matrix) {}
		SceneMatrix(const std::function<glm::mat4(const double)> func) : 
			func_(func) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override;

	public:
		void SetParent(const std::shared_ptr<Scene>& parent) override
		{
			parent_ = parent;
		}
		const std::shared_ptr<Scene>& GetParent() const override 
		{ 
			return parent_; 
		}
		bool IsLeaf() const override { return false; }
		bool IsRoot() const override { return !parent_; }

	private:
		glm::mat4 matrix_ = {};
		std::shared_ptr<Scene> parent_ = nullptr;
		std::function<glm::mat4(const double)> func_ = 
			[this](const double) { return matrix_; };
	};

	class SceneMesh : public Scene
	{
	public:
		SceneMesh(std::shared_ptr<sgl::Mesh> mesh) : mesh_(mesh) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override; 

	public:
		void SetParent(const std::shared_ptr<Scene>& parent) override
		{
			parent_ = parent;
		}
		const std::shared_ptr<Scene>& GetParent() const override
		{
			return parent_;
		}
		bool IsLeaf() const override { return true; }
		bool IsRoot() const override { return !parent_; }

	private:
		std::shared_ptr<sgl::Mesh> mesh_ = nullptr;
		std::shared_ptr<Scene> parent_ = nullptr;
	};

	class SceneTree : public std::vector<std::shared_ptr<Scene>>
	{
	public:
		void AddNode(
			const std::shared_ptr<Scene>& node, 
			const std::shared_ptr<Scene>& parent = nullptr);
		const std::shared_ptr<Scene> GetRoot() const;
	};

} // End namespace sgl.
