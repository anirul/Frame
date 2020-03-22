#include "Scene.h"
#include <stdexcept>

namespace sgl {

	void SceneMatrix::SetParent(const std::shared_ptr<Scene>& parent)
	{
		parent_ = parent;
	}

	const sgl::matrix SceneMatrix::GetLocalModel(const double dt) const
	{
		if (parent_)
		{
			return parent_->GetLocalModel(dt) * func_(dt);
		}
		else
		{
			return func_(dt);
		}
	}

	const std::shared_ptr<sgl::Mesh> SceneMatrix::GetLocalMesh() const
	{
		return nullptr;
	}

	void SceneMesh::SetParent(const std::shared_ptr<Scene>& parent)
	{
		parent_ = parent;
	}

	const sgl::matrix SceneMesh::GetLocalModel(const double dt) const
	{
		if (parent_)
		{
			return parent_->GetLocalModel(dt);
		}
		else
		{
			return matrix{};
		}
	}

	const std::shared_ptr<sgl::Mesh> SceneMesh::GetLocalMesh() const
	{
		return mesh_;
	}

	void SceneTree::AddNode(
		const std::shared_ptr<Scene>& node, 
		const std::shared_ptr<Scene>& parent /*= nullptr*/)
	{
		node->SetParent(parent);
		push_back(node);
	}

} // End namespace sgl.
