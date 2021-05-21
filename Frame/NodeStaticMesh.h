#pragma once

#include <memory>
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeStaticMesh : public NodeInterface
	{
	public:
		NodeStaticMesh(
			std::function<Ptr(const std::string&)> func,
			EntityId static_mesh_id, 
			EntityId material_id) :
			static_mesh_id_(static_mesh_id),
			material_id_(material_id) 
			{
				SetCallback(func);
			}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;

	public:
		const EntityId GetLocalMesh() const override { return static_mesh_id_; }
		EntityId GetMaterialId() const { return material_id_; }
		void SetMaterialId(EntityId id) { material_id_ = id; }

	private:
		EntityId static_mesh_id_ = 0;
		EntityId material_id_ = 0;
	};

} // End namespace frame.
