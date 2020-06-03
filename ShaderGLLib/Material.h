#pragma once

#include <iostream>
#include <string>
#include "../ShaderGLLib/Texture.h"

namespace sgl {

	// Specialized version of a texture manager.
	class Material : public TextureManager
	{
	public:
		// Default constructor (this will do NOTHING!).
		Material() = default;
		Material(const Material&) = default;
		// Parse from a MTL file.
		Material(std::istream& is, const std::string& name);

	public:
		// Suppose to return a list of string of the needed textures.
		const std::vector<std::string> GetTextures() const;
		// Update a texture manager with the texture contains in the material 
		// and the Material with the texture manager missing texture.
		void UpdateTextureManager(TextureManager& texture_manager);

	protected:
		std::shared_ptr<Texture> GetTextureFromFile(
			std::istream& is, 
			const std::string& stream_name,
			const std::string& element_name) const;
		std::shared_ptr<Texture> GetTextureFrom3Float(
			std::istream& is,
			const std::string& stream_name,
			const std::string& element_name) const;
		std::shared_ptr<Texture> GetTextureFrom1Float(
			std::istream& is,
			const std::string& stream_name,
			const std::string& element_name) const;
	};

	// Load a texture set from a MTL file.
	std::map<std::string, std::shared_ptr<Material>> LoadMaterialFromMtlStream(
		std::istream& is,
		const std::string& name);

} // End namespace sgl.