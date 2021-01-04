#include "Material.h"
#include <sstream>
#include <cassert>
#include "Image.h"

namespace frame::opengl {

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
				AddTexture("Color", LoadTextureFromFile(iss, name, dump));
			}
			if (dump == "Ka")
			{
				AddTexture("Color", LoadTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_Kd")
			{
				AddTexture("Color", LoadTextureFromFile(iss, name, dump));
			}
			if (dump == "Kd")
			{
				AddTexture("Color", LoadTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_norm")
			{
				AddTexture("Normal", LoadTextureFromFile(iss, name, dump));
			}
			if (dump == "norm")
			{
				AddTexture("Normal", LoadTextureFrom3Float(iss, name, dump));
			}
			if (dump == "map_Pm")
			{
				AddTexture("Metallic", LoadTextureFromFile(iss, name, dump));
			}
			if (dump == "Pm")
			{
				AddTexture("Metallic", LoadTextureFrom1Float(iss, name, dump));
			}
			if (dump == "map_Pr")
			{
				AddTexture("Roughness", LoadTextureFromFile(iss, name, dump));
			}
			if (dump == "Pr")
			{
				AddTexture("Roughness", LoadTextureFrom1Float(iss, name, dump));
			}
			// TODO(anirul): Implement the "d" and "illum" and all others.
		}
		if (!HasTexture("AmbientOcclusion"))
		{
			float single_pixel[3] = { 1.f, 1.f, 1.f };
			AddTexture("AmbientOcclusion", std::make_shared<Texture>(
				std::pair{ 1, 1 },
				single_pixel,
				sgl::PixelElementSize_FLOAT()));
		}
	}

	Material::Material(const frame::proto::Material& material)
	{
		effect_name_ = material.effect_name();
		for (const auto& texture_name : material.texture_names())
			AddTexture(texture_name);
	}

	Material Material::operator+(const Material& material)
	{
		Material ret_material(*this);
		ret_material += material;
		return ret_material;
	}

	Material& Material::operator+=(const Material& material)
	{
		for (const auto& texture_name : material.GetTextureNames())
		{
			AddTexture(texture_name);
		}
		return *this;
	}

	Material::~Material()
	{
		for (const auto& name : name_array_)
		{
			if (!name.empty())
			{
				throw std::runtime_error(
					"Material [" + name + "] has not been freed!");
			}
		}
	}

	bool Material::AddTexture(const std::string& name)
	{
		RemoveTexture(name);
		auto ret = texture_names_.insert(name);
		return ret.second;
	}

	bool Material::HasTexture(const std::string& name) const
	{
		return texture_names_.find(name) != texture_names_.end();
	}

	bool Material::RemoveTexture(const std::string& name)
	{
		// Check if present in the name texture map.
		auto it = texture_names_.find(name);
		if (it == texture_names_.end())
		{
			return false;
		}

		// Remove it.
		texture_names_.erase(it);
		return true;
	}

	const int Material::EnableTexture(
		const std::string& name, 
		const std::shared_ptr<TextureInterface>& texture) const
	{
		auto it1 = texture_names_.find(name);
		if (it1 == texture_names_.end())
		{
			throw std::runtime_error("try to enable a texture: " + name);
		}
		for (int i = 0; i < name_array_.size(); ++i)
		{
			if (name_array_[i].empty())
			{
				name_array_[i] = name;
				texture->Bind(i);
				return i;
			}
		}
		throw std::runtime_error("No free slots!");
	}

	void Material::DisableTexture(
		const std::string& name,
		const std::shared_ptr<TextureInterface>& texture) const
	{
		auto it1 = texture_names_.find(name);
		if (it1 == texture_names_.end())
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
			texture->UnBind();
		}
		else
		{
			throw std::runtime_error("No slot bind to: " + name);
		}
	}

	void Material::DisableAll(
		const std::map<std::string, std::shared_ptr<TextureInterface>>& 
			texture_map) const
	{
		for (int i = 0; i < 32; ++i)
		{
			if (!name_array_[i].empty())
			{
				auto it = texture_map.find(name_array_[i]);
				if (it == texture_map.cend())
				{
					throw std::runtime_error(
						"could not find texture: " + name_array_[i]);
				}
				DisableTexture(it->first, it->second);
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

} // End namespace frame::opengl.
