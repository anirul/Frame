#pragma once

#include <memory>
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeStaticMesh : public NodeInterface
	{
	public:
		NodeStaticMesh(
			EntityId static_mesh_id, 
			EntityId material_id) :
			static_mesh_id_(static_mesh_id),
			material_id_(material_id) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const EntityId GetLocalMesh() const override;

	public:
		EntityId GetMaterialId() const { return material_id_; }
		void SetMaterialId(EntityId id) { material_id_ = id; }

	private:
		EntityId static_mesh_id_ = 0;
		EntityId material_id_ = 0;
	};

} // End namespace frame.
