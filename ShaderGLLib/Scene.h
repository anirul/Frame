#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include "../ShaderGLLib/Vector.h"
#include "../ShaderGLLib/Mesh.h"

namespace sgl {

	struct Scene 
	{
		virtual void SetParent(const std::shared_ptr<Scene>& parent) = 0;
		virtual const std::shared_ptr<Scene>& GetParent() const = 0;
		virtual const matrix GetLocalModel(const double dt) const = 0;
		virtual const std::shared_ptr<Mesh> GetLocalMesh() const = 0;
		virtual bool IsLeaf() const = 0;
		virtual bool IsRoot() const = 0;
	};

	class SceneMatrix : public Scene 
	{
	public:
		SceneMatrix(const matrix& matrix) : matrix_(matrix) {}
		SceneMatrix(const std::function<matrix(const double)> func) : 
			func_(func) {}
		void SetParent(const std::shared_ptr<Scene>& parent) override
		{
			parent_ = parent;
		}
		const std::shared_ptr<Scene>& GetParent() const override 
		{ 
			return parent_; 
		}
		const matrix GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override;
		bool IsLeaf() const override { return false; }
		bool IsRoot() const override { return !parent_; }

	private:
		sgl::matrix matrix_ = {};
		std::shared_ptr<Scene> parent_ = nullptr;
		std::function<matrix(const double)> func_ = 
			[this](const double) { return matrix_; };
	};

	class SceneMesh : public Scene
	{
	public:
		SceneMesh(std::shared_ptr<sgl::Mesh> mesh) : mesh_(mesh) {}
		void SetParent(const std::shared_ptr<Scene>& parent) override
		{
			parent_ = parent;
		}
		const std::shared_ptr<Scene>& GetParent() const override
		{
			return parent_;
		}
		const matrix GetLocalModel(const double dt) const override;
		const std::shared_ptr<Mesh> GetLocalMesh() const override;
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
