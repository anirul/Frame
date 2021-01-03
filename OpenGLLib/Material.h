#pragma once

#include <iostream>
#include <set>
#include <string>
#include "../Frame/MaterialInterface.h"
#include "../OpenGLLib/Texture.h"

namespace frame::opengl {

	// This is the texture manager, it is suppose to be handling the textures
	// for a single model (mesh), this is also suppose to have an effect (or
	// a program).
	class Material : public MaterialInterface
	{
	public:
		// Default constructor (this will do NOTHING!).
		Material() = default;
		Material(const Material&) = default;
		Material(const frame::proto::Material& material);
		virtual ~Material();
		// Parse from a MTL file.
		Material(std::istream& is, const std::string& name);

	public:
		Material operator+(const Material& material);
		Material& operator+=(const Material& material);

	public:
		// Texture management part.
		bool AddTexture(const std::string& name);
		bool HasTexture(const std::string& name) const;
		bool RemoveTexture(const std::string& name);
		// Return the binding slot of the texture (to be passed to the program).
		const int EnableTexture(
			const std::string& name, 
			const std::shared_ptr<TextureInterface>& texture) const;
		void DisableTexture(
			const std::string& name,
			const std::shared_ptr<TextureInterface>& texture) const;
		void DisableAll(
			const std::map<std::string, std::shared_ptr<TextureInterface>>&
				texture_map) const;

	public:
		// Texture management part.
		bool AddTextureId(std::uint64_t id) override;
		bool HasTextureId(std::uint64_t id) const override;
		bool RemoveTextureId(std::uint64_t id) override;
		// Return the binding slot of a texture (to be passed to the program).
		const int EnableTexture(
			const std::shared_ptr<TextureInterface> texture) const override;
		// Unbind the texture and remove it from the list.
		void DisableTexture(
			const std::shared_ptr<TextureInterface> texture) const override;
		// Disable all the texture and unbind them.
		void DisableAll(
			const std::vector<std::shared_ptr<TextureInterface>> textures)
			const override;

	public:
		const std::set<std::string>& GetTextureNames() const 
		{
			return texture_names_;
		}
		void SetEffectName(const std::string& name) { effect_name_ = name; }
		const std::string GetEffectName() const { return effect_name_; }

	private:
		std::string effect_name_;
		std::set<std::string> texture_names_ = {};
		mutable std::array<std::string, 32> name_array_ = {};
	};

	// Load a texture set from a MTL file.
	std::map<std::string, std::shared_ptr<Material>> LoadMaterialFromMtlStream(
		std::istream& is,
		const std::string& name);

} // End namespace frame::opengl.
