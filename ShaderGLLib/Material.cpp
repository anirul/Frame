#include "Material.h"
#include <sstream>
#include <cassert>
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

	Material Material::operator+(const Material& material)
	{
		Material ret_material(*this);
		ret_material += material;
		return ret_material;
	}

	Material& Material::operator+=(const Material& material)
	{
		for (const auto& value : material.name_texture_map_)
		{
			name_texture_map_.insert({ value.first, value.second });
		}
		return *this;
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

	Material::~Material()
	{
		DisableAll();
	}

	bool Material::AddTexture(
		const std::string& name,
		const std::shared_ptr<sgl::Texture>& texture)
	{
		RemoveTexture(name);
		auto ret = name_texture_map_.insert({ name, texture });
		return ret.second;
	}

	const std::shared_ptr<sgl::Texture>& Material::GetTexture(
		const std::string& name) const
	{
		auto it = name_texture_map_.find(name);
		if (it == name_texture_map_.end())
		{
			throw std::runtime_error("No such texture: " + name);
		}
		return it->second;
	}

	bool Material::HasTexture(const std::string& name) const
	{
		return name_texture_map_.find(name) != name_texture_map_.end();
	}

	bool Material::RemoveTexture(const std::string& name)
	{
		// Check if present in the name texture map.
		auto it = name_texture_map_.find(name);
		if (it == name_texture_map_.end())
		{
			return false;
		}

		// Remove it.
		name_texture_map_.erase(it);
		return true;
	}

	const int Material::EnableTexture(const std::string& name) const
	{
		auto it1 = name_texture_map_.find(name);
		if (it1 == name_texture_map_.end())
		{
			throw std::runtime_error("try to enable a texture: " + name);
		}
		for (int i = 0; i < name_array_.size(); ++i)
		{
			if (name_array_[i].empty())
			{
				name_array_[i] = name;
				it1->second->Bind(i);
				return i;
			}
		}
		throw std::runtime_error("No free slots!");
	}

	void Material::DisableTexture(const std::string& name) const
	{
		auto it1 = name_texture_map_.find(name);
		if (it1 == name_texture_map_.end())
		{
			throw std::runtime_error("no texture named: " + name);
		}
		auto it2 = std::find_if(
			name_array_.begin(),
			name_array_.end(),
			[name](const std::string& value)
		{
			return value == name;
		});
		if (it2 != name_array_.end())
		{
			*it2 = "";
			it1->second->UnBind();
		}
		else
		{
			throw std::runtime_error("No slot bind to: " + name);
		}
	}

	void Material::DisableAll() const
	{
		for (int i = 0; i < 32; ++i)
		{
			if (!name_array_[i].empty())
			{
				DisableTexture(name_array_[i]);
			}
		}
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
