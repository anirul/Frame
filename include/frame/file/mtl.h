#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "frame/logger.h"

namespace frame::file
{

// Structure to hold the material properties from the MTL file.
struct MtlMaterial
{
    std.string name;
    std::string ambient_str;
    glm::vec4 ambient_vec4;
    std::string diffuse_str;
    glm::vec4 diffuse_vec4;
    std::string displacement_str;
    std::string emmissive_str;
    std::string metallic_str;
    float metallic_val;
    std::string normal_str;
    std::string roughness_str;
    float roughness_val;
    std::string sheen_str;
    float sheen_val;
};

// Mtl class to parse and store material information from a .mtl file.
class Mtl
{
  public:
    // Constructor that takes the path to the .mtl file and a list of search
    // paths for textures.
    Mtl(const std::filesystem::path& path,
        const std::vector<std::filesystem::path>& search_paths = {});
    // Get a material by name.
    const MtlMaterial& GetMaterial(const std::string& name) const;
    // Get all materials.
    const std::vector<MtlMaterial>& GetMaterials() const;

  private:
    std::vector<MtlMaterial> materials_;
    std::map<std::string, std::size_t> material_name_map_;
    Logger& logger_ = Logger::GetInstance();
};

} // End namespace frame::file.
