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
		Material(const std::shared_ptr<LevelInterface> level) : 
			level_(level) {}
		virtual ~Material();

	public:
		// Texture management part.
		bool AddTextureId(std::uint64_t id, const std::string& name) override;
		bool HasTextureId(std::uint64_t id) const override;
		bool RemoveTextureId(std::uint64_t id) override;
		// Return the list of texture ids.
		const std::vector<std::uint64_t> GetIds() const;
		// Return the name and the binding slot of a texture (to be passed to
		// the program).
		const std::pair<std::string, int> EnableTextureId(
			std::uint64_t id) const override;
		// Unbind the texture and remove it from the list.
		void DisableTextureId(std::uint64_t) const override;
		// Disable all the texture and unbind them.
		void DisableAll() const override;

	private:
		std::shared_ptr<LevelInterface> level_ = nullptr;
		std::map<std::uint64_t, std::string> id_name_map_ = {};
		mutable std::array<std::uint64_t, 32> id_array_ = {};
	};

} // End namespace frame::opengl.
