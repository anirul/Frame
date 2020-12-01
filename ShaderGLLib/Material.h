#pragma once

#include <iostream>
#include <string>
#include "../ShaderGLLib/Texture.h"

namespace sgl {

	// This is the texture manager, it is suppose to be handling the textures
	// for a single model (mesh).
	class Material
	{
	public:
		// Default constructor (this will do NOTHING!).
		Material() = default;
		Material(const Material&) = default;
		virtual ~Material();
		// Parse from a MTL file.
		Material(std::istream& is, const std::string& name);
		Material operator+(const Material& material);
		Material& operator+=(const Material& material);

	public:
		// Texture management part.
		bool AddTexture(
			const std::string& name,
			const std::shared_ptr<sgl::Texture> texture);
		const std::shared_ptr<sgl::Texture> GetTexture(
			const std::string& name) const;
		bool HasTexture(const std::string& name) const;
		bool RemoveTexture(const std::string& name);
		// Return the binding slot of the texture (to be passed to the program).
		const int EnableTexture(const std::string& name) const;
		void DisableTexture(const std::string& name) const;
		void DisableAll() const;

	public:
		const std::map<std::string, std::shared_ptr<Texture>>& GetMap() const
		{
			return name_texture_map_;
		}

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

	private:
		std::map<std::string, std::shared_ptr<Texture>> name_texture_map_ = {};
		mutable std::array<std::string, 32> name_array_ = {};
	};

	// Load a texture set from a MTL file.
	std::map<std::string, std::shared_ptr<Material>> LoadMaterialFromMtlStream(
		std::istream& is,
		const std::string& name);

} // End namespace sgl.