#include "Material.h"
#include <sstream>
#include <cassert>
#include "Image.h"

namespace sgl {	

	namespace {
		const std::vector<std::string> texture_vec = {
				"Color",
				"Normal",
				"Metallic",
				"Roughness",
				"AmbientOcclusion",
				"MonteCarloPrefilter",
				"Irradiance",
				"IntegrateBRDF"
		};
	}

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
				AddTexture("Color", GetTextureFromFile(iss, name, dump));
			}
			if (dump == "Ka")
			{
				AddTexture("Color", GetTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_Kd")
			{
				AddTexture("Color", GetTextureFromFile(iss, name, dump));
			}
			if (dump == "Kd")
			{
				AddTexture("Color", GetTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_norm")
			{
				AddTexture("Normal", GetTextureFromFile(iss, name, dump));
			}
			if (dump == "norm")
			{
				AddTexture("Normal", GetTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_Pm")
			{
				AddTexture("Metallic", GetTextureFromFile(iss, name, dump));
			}
			if (dump == "Pm")
			{
				AddTexture("Metallic", GetTextureFrom1Float(iss, name, dump));
			}
			if (dump == "map_Pr")
			{
				AddTexture("Roughness", GetTextureFromFile(iss, name, dump));
			}
			if (dump == "Pr")
			{
				AddTexture("Roughness", GetTextureFrom1Float(iss, name, dump));
			}
			// TODO(anirul): Implement the "d" and "illum" and all others.
		}
		if (!HasTexture("AmbientOcclusion"))
		{
			float single_pixel[3] = { 1.f, 1.f, 1.f };
			AddTexture("AmbientOcclusion", std::make_shared<Texture>(
				std::pair{ 1, 1 },
				single_pixel,
				sgl::PixelElementSize::FLOAT));
		}
	}

	const std::vector<std::string> Material::GetTextures() const
	{
		for (const auto& str : texture_vec)
		{
			if (!HasTexture(str))
			{
				throw std::runtime_error("Couldn't get texture: " + str);
			}
		}
		return texture_vec;
	}

	void Material::UpdateTextureManager(TextureManager& texture_manager)
	{
		for (const auto& str: texture_vec)
		{
			if (!HasTexture(str))
			{
				if (texture_manager.HasTexture(str))
				{
					AddTexture(str, texture_manager.GetTexture(str));
				}
				else
				{
					throw std::runtime_error(
						"Texture: " + str + 
						" is neither in the material nor in the texture " +
						"manager?");
				}
			}
		}
		auto textures_names_vec  = GetTexturesNames();
		for (const auto& str : textures_names_vec)
		{
			texture_manager.AddTexture(str, GetTexture(str));
		}
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFromFile(
		std::istream& is, 
		const std::string& stream_name,
		const std::string& element_name) const
	{
		std::string file_name;
		if (!(is >> file_name))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" no file name at " +
				element_name);
		}
		return std::make_shared<Texture>(file_name);
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFrom3Float(
		std::istream& is, 
		const std::string& stream_name,
		const std::string& element_name) const
	{
		float rgb[3] = { 0.f, 0.f, 0.f };
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
		return std::make_shared<Texture>(
			std::pair{ 1, 1 },
			rgb,
			PixelElementSize::FLOAT,
			PixelStructure::RGB);
	}

	std::shared_ptr<sgl::Texture> Material::GetTextureFrom1Float(
		std::istream& is, 
		const std::string& stream_name, 
		const std::string& element_name) const
	{
		float grey[1] = { 0.f };
		if (!(is >> grey[0]))
		{
			throw std::runtime_error(
				"Error parsing file: " +
				stream_name +
				" could not get the r value for " +
				element_name);
		}
		return std::make_shared<Texture>(
			std::pair{ 1, 1 },
			grey,
			PixelElementSize::FLOAT,
			PixelStructure::GREY);
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
			[&material_map, &mtl_part, &name_extended, &mtl_name]()
		{
			assert(!mtl_name.empty());
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
			if (line[0] == '#') continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + name + " no token found.");
			}
			if (dump == "newmtl")
			{
				if (!mtl_part.empty() && !mtl_name.empty()) 
				{
					lambda_create_material();
				}
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
		if (!mtl_part.empty() && !mtl_name.empty()) lambda_create_material();
		return material_map;
	}

} // End namespace sgl.
