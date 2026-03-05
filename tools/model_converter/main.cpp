#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace
{

int ConvertOne(const std::filesystem::path& input_path,
               const std::filesystem::path& output_path)
{
    Assimp::Importer importer;
    constexpr unsigned int kImportFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(input_path.string(), kImportFlags);
    if (!scene)
    {
        std::cerr << "Failed to import '" << input_path.string()
                  << "': " << importer.GetErrorString() << std::endl;
        return 1;
    }

    Assimp::Exporter exporter;
    const aiReturn result =
        exporter.Export(scene, "glb2", output_path.string(), 0u);
    if (result != aiReturn_SUCCESS)
    {
        std::cerr << "Failed to export '" << output_path.string()
                  << "': " << exporter.GetErrorString() << std::endl;
        return 1;
    }
    std::cout << "Converted: " << input_path.string()
              << " -> " << output_path.string() << std::endl;
    return 0;
}

} // namespace

int main(int argc, char** argv)
try
{
    if (argc != 3)
    {
        std::cerr << "Usage: FrameModelConverter <input_model> <output_glb>"
                  << std::endl;
        return 1;
    }
    const std::filesystem::path input_path = argv[1];
    const std::filesystem::path output_path = argv[2];
    if (!std::filesystem::exists(input_path))
    {
        throw std::runtime_error(
            "Input file does not exist: " + input_path.string());
    }
    std::filesystem::create_directories(output_path.parent_path());
    return ConvertOne(input_path, output_path);
}
catch (const std::exception& exception)
{
    std::cerr << exception.what() << std::endl;
    return 1;
}

