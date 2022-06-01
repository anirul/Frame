#pragma once

#include <memory>
#include "Frame/EntityId.h"
#include "Frame/NameInterface.h"

namespace frame {

	// Static mesh interface (proposal).
	// Id are in EntityId to allow for more see the level interface.
	class StaticMeshInterface : public NameInterface
	{
	public:
		virtual ~StaticMeshInterface() = default;
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
		// Name interface
		std::string GetName() const override = 0;
		void SetName(const std::string& name) override = 0;
	};

} // End namespace frame.
