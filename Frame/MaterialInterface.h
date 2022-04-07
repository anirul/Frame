#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Frame/EntityId.h"
#include "Frame/NameInterface.h"

namespace frame {

	class LevelInterface;

	struct MaterialInterface : public NameInterface
	{
		// Get the program that is sued by this material.
		virtual EntityId GetProgramId(
			const LevelInterface* level = nullptr) const = 0;
		// Set the program Id.
		virtual void SetProgramId(EntityId id) = 0;
		virtual void SetProgramName(const std::string& name) = 0;
		// Texture management part.
		virtual bool AddTextureId(
			EntityId id, 
			const std::string& name) = 0;
		virtual bool HasTextureId(EntityId id) const = 0;
		virtual bool RemoveTextureId(EntityId id) = 0;
		// Return the list of texture ids.
		virtual const std::vector<EntityId> GetIds() const = 0;
		// Return the name and the binding slot of a texture (to be passed to
		// the program).
		virtual const std::pair<std::string, int> EnableTextureId(
			EntityId id) const = 0;
		// Unbind the texture and remove it from the list.
		virtual void DisableTextureId(EntityId id) const = 0;
		// Disable all the texture and unbind them.
		virtual void DisableAll() const = 0;
		virtual ~MaterialInterface() = default;
	};

} // End namespace frame.
