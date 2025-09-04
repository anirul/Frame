#include "frame/file/mtl.h"

#include <fstream>
#include <sstream>

#include <glm/glm.hpp>

#include "frame/file/file_system.h"

namespace frame::file
{

Mtl::Mtl(const std::filesystem::path& path,
         const std::vector<std::filesystem::path>& search_paths)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error(
            std::format("Could not open file: {}", path.string()));
    }

    std::string line;
    MtlMaterial* current_material = nullptr;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "newmtl")
        {
            std::string name;
            iss >> name;
            materials_.emplace_back();
            current_material = &materials_.back();
            current_material->name = name;
            material_name_map_[name] = materials_.size() - 1;
        }
        else if (current_material)
        {
            if (keyword == "map_Ka")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->ambient_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "Ka")
            {
                iss >> current_material->ambient_vec4.r >>
                    current_material->ambient_vec4.g >>
                    current_material->ambient_vec4.b;
            }
            else if (keyword == "map_Kd")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->diffuse_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "Kd")
            {
                iss >> current_material->diffuse_vec4.r >>
                    current_material->diffuse_vec4.g >>
                    current_material->diffuse_vec4.b;
            }
            else if (keyword == "map_disp" || keyword == "disp")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->displacement_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "map_Ke" || keyword == "Ke")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->emmissive_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "map_Pm" || keyword == "Pm")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->metallic_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "refl")
            {
                // Not supported yet.
            }
            else if (keyword == "norm")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->normal_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "map_Pr" || keyword == "Pr")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->roughness_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "map_Ps" || keyword == "Ps")
            {
                std::string texture_path;
                iss >> texture_path;
                current_material->sheen_str =
                    FindFile(texture_path, search_paths).string();
            }
            else if (keyword == "d")
            {
                // alpha
                iss >> current_material->diffuse_vec4.a;
                current_material->ambient_vec4.a =
                    current_material->diffuse_vec4.a;
            }
        }
    }
}

const MtlMaterial& Mtl::GetMaterial(const std::string& name) const
{
    if (material_name_map_.count(name))
    {
        return materials_[material_name_map_.at(name)];
    }
    throw std::runtime_error(std::format("Material not found: {}", name));
}

const std::vector<MtlMaterial>& Mtl::GetMaterials() const
{
    return materials_;
}

} // End namespace frame::file.
