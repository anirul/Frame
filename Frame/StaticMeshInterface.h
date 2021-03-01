#pragma once

#include <memory>
#include "Frame/BindInterface.h"
#include "Frame/EntityId.h"

namespace frame {

	// Static mesh interface (proposal).
	// Id are in EntityId to allow for more see the level interface.
	struct StaticMeshInterface : public BindInterface
	{
		virtual void SetMaterialId(EntityId id) = 0;
		virtual EntityId GetMaterialId() const = 0;
		virtual EntityId GetPointBufferId() const = 0;
		virtual EntityId GetNormalBufferId() const = 0;
		virtual EntityId GetTextureBufferId() const = 0;
		virtual EntityId GetIndexBufferId() const = 0;
		virtual std::size_t GetIndexSize() const = 0;
		virtual bool IsClearBuffer() const = 0;
	};

} // End namespace frame.
