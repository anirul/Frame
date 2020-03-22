#include "Scene.h"
#include <stdexcept>

namespace sgl {

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

	const std::shared_ptr<Scene> SceneTree::GetRoot() const
	{
		std::shared_ptr<Scene> ret;
		for (const auto& scene : *this)
		{
			if (!scene->GetParent())
			{
				if (ret)
				{
					throw std::runtime_error("More than one root?");
				}
				ret = scene;
			}
		}
		return ret;
	}

} // End namespace sgl.
