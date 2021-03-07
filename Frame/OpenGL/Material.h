#pragma once

#include <iostream>
#include <set>
#include <string>
#include "Frame/LevelInterface.h"
#include "Frame/MaterialInterface.h"
#include "Frame/OpenGL/Texture.h"

namespace frame::opengl {

	// This is the texture manager, it is suppose to be handling the textures
	// for a single model (mesh), this is also suppose to have an effect (or
	// a program).
	class Material : public MaterialInterface
	{
	public:
		Material() = default;
		virtual ~Material();

	public:
		// Texture management part.
		bool AddTextureId(EntityId id, const std::string& name) override;
		bool HasTextureId(EntityId id) const override;
		bool RemoveTextureId(EntityId id) override;
		// Return the list of texture ids.
		const std::vector<EntityId> GetIds() const final;
		// Return the name and the binding slot of a texture (to be passed to
		// the program).
		const std::pair<std::string, int> EnableTextureId(
			EntityId id) const override;
		// Unbind the texture and remove it from the list.
		void DisableTextureId(EntityId id) const override;
		// Disable all the texture and unbind them.
		void DisableAll() const override;

	private:
		std::map<EntityId, std::string> id_name_map_ = {};
		mutable std::array<EntityId, 32> id_array_ = {};
	};

} // End namespace frame::opengl.
