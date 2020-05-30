#pragma once

#include <iostream>
#include <string>
#include "../ShaderGLLib/Texture.h"

namespace sgl {

	class Material 
	{
	public:
		Material(std::istream& is, const std::string& name);
		Material(
			const std::shared_ptr<Texture>& color,
			const std::shared_ptr<Texture>& normal,
			const std::shared_ptr<Texture>& metal,
			const std::shared_ptr<Texture>& roughness);
		Material(
			const std::shared_ptr<Texture>& color,
			const std::shared_ptr<Texture>& normal,
			const std::shared_ptr<Texture>& metal,
			const std::shared_ptr<Texture>& roughness,
			const std::shared_ptr<Texture>& ambient_occlusion);

	public:
		const std::shared_ptr<Texture>& GetColor() const { return color_; }
		const std::shared_ptr<Texture>& GetNormal() const { return normal_; }
		const std::shared_ptr<Texture>& GetMetal() const { return metal_; }
		const std::shared_ptr<Texture>& GetRoughness() const 
		{ 
			return roughness_; 
		}
		const std::shared_ptr<Texture>& GetAmbientOcclusion() const
		{
			return ambient_occlusion_;
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
		std::shared_ptr<Texture> color_ = nullptr;
		std::shared_ptr<Texture> normal_ = nullptr;
		std::shared_ptr<Texture> metal_ = nullptr;
		std::shared_ptr<Texture> roughness_ = nullptr;
		std::shared_ptr<Texture> ambient_occlusion_ = nullptr;
	};

	using MaterialMap = std::map<std::string, std::shared_ptr<Material>>;

} // End namespace sgl.