#pragma once

#include <memory>
#include "Frame/BindInterface.h"
#include "Frame/EntityId.h"

namespace frame {

	// Static mesh interface (proposal).
	// Id are in EntityId to allow for more see the level interface.
	struct StaticMeshInterface : public BindInterface
	{
		// TODO(anirul): Move material to node static mesh.
		virtual void SetMaterialId(EntityId id) = 0;
		virtual EntityId GetMaterialId() const = 0;
		// Buffer management.
		virtual EntityId GetPointBufferId() const = 0;
		virtual EntityId GetNormalBufferId() const = 0;
		virtual EntityId GetTextureBufferId() const = 0;
		virtual EntityId GetIndexBufferId() const = 0;
		// This is the size in bytes! so if you need the element size just
		// divide this number by the sizeof(std::int32_t).
		virtual std::size_t GetIndexSize() const = 0;
		virtual bool IsClearBuffer() const = 0;
	};

} // End namespace frame.
