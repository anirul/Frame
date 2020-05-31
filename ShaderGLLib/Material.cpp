#include "Material.h"
#include <sstream>
#include "Image.h"

namespace sgl {	

	Material::Material(std::istream& is, const std::string& name)
	{
		while (!is.eof())
		{
			std::string line = "";
			if (!std::getline(is, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + name + " no token found.");
			}
			if (dump == "map_Ka") 
			{
				if (!color_) color_ = GetTextureFromFile(iss, name, dump);
			}
			if (dump == "Ka")
			{
				if (!color_) color_ = GetTextureFrom3Float(iss, name, dump);
			}
			if (dump == "map_Kd")
			{
				if (!color_) color_ = GetTextureFromFile(iss, name, dump);
			}
			if (dump == "Kd")
			{
				if (!color_) color_ = GetTextureFrom3Float(iss, name, dump);
			}
			if (dump == "map_norm")
			{
				if (!normal_) normal_ = GetTextureFromFile(iss, name, dump);
			}
			if (dump == "norm")
			{
				if (!normal_) normal_ = GetTextureFrom3Float(iss, name, dump);
			}
			if (dump == "map_Pm")
			{
				if (!metal_) metal_ = GetTextureFromFile(iss, name, dump);
			}
			if (dump == "Pm")
			{
				if (!metal_) metal_ = GetTextureFrom1Float(iss, name, dump);
			}
			if (dump == "map_Pr")
			{
				if (!roughness_) 
					roughness_ = GetTextureFromFile(iss, name, dump);
			}
			if (dump == "Pr")
			{
				if (!roughness_) 
					roughness_ = GetTextureFrom1Float(iss, name, dump);
			}
			// TODO(anirul) Implement the "d" and "illum" and all others.
		}
	}

	Material::Material(
		const std::shared_ptr<Texture>& color, 
		const std::shared_ptr<Texture>& normal, 
		const std::shared_ptr<Texture>& metal, 
		const std::shared_ptr<Texture>& roughness)
	{
		color_ = color;
		normal_ = normal;
		metal_ = metal;
		roughness_ = roughness;
		if (ambient_occlusion_) return;
		unsigned char single_pixel[3] = { 255, 255, 255 };
		ambient_occlusion_ = std::make_shared<Texture>(
			std::pair{1, 1},
			&single_pixel,
			sgl::PixelElementSize::BYTE);
	}

	Material::Material(
		const std::shared_ptr<Texture>& color, 
		const std::shared_ptr<Texture>& normal, 
		const std::shared_ptr<Texture>& metal, 
		const std::shared_ptr<Texture>& roughness, 
		const std::shared_ptr<Texture>& ambient_occlusion) :
		Material(color, normal, metal, roughness)
	{
		ambient_occlusion_ = ambient_occlusion;
	}

	std::vector<std::string> Material::GetTextures() const
	{
		return { "Color", "Normal", "Metal", "Roughness", "AmbientOcclusion" };
	}

	TextureManager Material::GetTextureManager() const
	{
		TextureManager texture_manager;
		texture_manager.AddTexture("Color", color_);
		texture_manager.AddTexture("Normal", normal_);
		texture_manager.AddTexture("Metal", metal_);
		texture_manager.AddTexture("Roughness", roughness_);
		texture_manager.AddTexture("AmbientOcclusion", ambient_occlusion_);
		return texture_manager;
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFromFile(
		std::istream& is, 
		const std::string& stream_name,
		const std::string& element_name) const
	{
		std::shared_ptr<Texture> texture = nullptr;
		std::string file_name;
		if (!(is >> file_name))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" no file name at " +
				element_name);
		}
		return texture;
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFrom3Float(
		std::istream& is, 
		const std::string& stream_name,
		const std::string& element_name) const
	{
		float rgb[3] = { 0, 0, 0 };
		if (!(is >> rgb[0]))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" could not get the r value for " +
				element_name);
		}
		if (!(is >> rgb[1]))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" could not get the g value for " +
				element_name);
		}
		if (!(is >> rgb[2]))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" could not get the b value for " +
				element_name);
		}
		auto texture = std::make_shared<Texture>(
			std::pair{ 1, 1 },
			&rgb,
			PixelElementSize::FLOAT,
			PixelStructure::RGB);
		return texture;
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFrom1Float(
		std::istream& is, 
		const std::string& stream_name, 
		const std::string& element_name) const
	{
		float grey[1] = { 0 };
		if (!(is >> grey[0]))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" could not get the r value for " +
				element_name);
		}
		auto texture = std::make_shared<Texture>(
			std::pair{ 1, 1 },
			&grey,
			PixelElementSize::FLOAT,
			PixelStructure::GREY);
		return texture;
	}

	std::map<std::string, std::shared_ptr<Material>> LoadMaterialFromMtlStream(
		std::istream& is, 
		const std::string& name)
	{
		std::map<std::string, std::shared_ptr<Material>> material_map{};
		std::string mtl_part = "";
		std::string name_extended = "";
		std::string mtl_name = "";
		auto lambda_create_material = 
			[&material_map, &mtl_part, &mtl_name, &name_extended]()
		{
			material_map.emplace(
				mtl_name,
				std::make_shared<Material>(
					std::istringstream(mtl_part),
					name_extended));
			mtl_part.clear();
		};
		while (!is.eof())
		{
			std::string line = "";
			if (!std::getline(is, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + name + " no token found.");
			}
			if (dump == "newmtl")
			{
				if (!mtl_part.empty()) lambda_create_material();
				if (!(iss >> mtl_name))
				{
					throw std::runtime_error(
						"Material should have name: " + name);
				}
				name_extended = name + ":" + mtl_name;
			}
			else
			{
				mtl_part += line + "\n";
			}
		}
		if (!mtl_part.empty()) lambda_create_material();
		return material_map;
	}

} // End namespace sgl.
