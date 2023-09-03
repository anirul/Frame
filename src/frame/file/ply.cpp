#include "frame/file/ply.h"

#include <happly.h>

#include <fstream>
#include <numeric>

namespace frame::file {

    namespace {

        // Template to replace <T> type with the correct float type.
        template <typename T>
        void GetElementInternal(
            happly::PLYData& ply,
            const std::string& name,
            int i,
            std::vector<std::vector<float>>& result)
        {
            if (ply.getElement("vertex").hasPropertyType<T>(name)) {
                try {
                    auto vec = ply.getElement("vertex").getProperty<T>(name);
                    result[i].resize(vec.size());
                    std::transform(vec.begin(), vec.end(), result[i].begin(),
                        [](T t) { return static_cast<float>(t); });
                }
                catch (const std::exception& e) {
                    Logger::GetInstance()->warn(e.what());
                }
            }
        }

        // Template Specialization for std::uint8_t type (/ 255).
        template <>
        void GetElementInternal<std::uint8_t>(
            happly::PLYData& ply,
            const std::string& name,
            int i,
            std::vector<std::vector<float>>& result)
        {
            if (ply.getElement("vertex").hasPropertyType<std::uint8_t>(name)) {
                try {
                    auto vec =
                        ply.getElement(
                            "vertex").getProperty<std::uint8_t>(name);
                    result[i].resize(vec.size());
                    std::transform(
                        vec.begin(), vec.end(), result[i].begin(),
                        [](std::uint8_t uc) {
                            return static_cast<float>(uc) / 255.0f;
                        });
                }
                catch (const std::exception& e) {
                    Logger::GetInstance()->warn(e.what());
                }
            }
        }

        // Template get float arrays of size <SIZE>.
        template <std::size_t SIZE>
        void GetElementFloat(
            happly::PLYData& ply,
            const std::array<std::string, SIZE>& names,
            std::vector<std::vector<float>>& result)
        {
            if (result.size() != SIZE)
                throw std::runtime_error(fmt::format(
                    "Result size is incorrect should be {} is {}",
                    SIZE, result.size()));
            for (int i = 0; i < SIZE; ++i) {
                GetElementInternal<std::uint8_t>(ply, names[i], i, result);
                GetElementInternal<float>(ply, names[i], i, result);
                GetElementInternal<double>(ply, names[i], i, result);
            }
        }

        std::vector<glm::vec2> GetElementVertexPropertyVec2(
            happly::PLYData& ply,
            const std::array<std::string, 2>& names)
        {
            std::vector<glm::vec2> result;
            std::vector<std::vector<float>> float_vec = {};
            float_vec.resize(2);
            GetElementFloat<2>(ply, names, float_vec);
            for (int i = 0; i < float_vec[0].size(); ++i) {
                result.emplace_back(
                    glm::vec2(float_vec[0][i], float_vec[1][i]));
            }
            return result;
        }

        std::vector<glm::vec3> GetElementVertexPropertyVec3(
            happly::PLYData& ply,
            const std::array<std::string, 3>& names)
        {
            std::vector<glm::vec3> result;
            std::vector<std::vector<float>> float_vec = {};
            float_vec.resize(3);
            GetElementFloat<3>(ply, names, float_vec);
            for (int i = 0; i < float_vec[0].size(); ++i) {
                result.emplace_back(
                    glm::vec3(
                        float_vec[0][i],
                        float_vec[1][i],
                        float_vec[2][i]));
            }
            return result;
        }

        std::vector<glm::vec4> GetElementVertexPropertyVec4(
            happly::PLYData& ply,
            const std::array<std::string, 4>& names)
        {
            std::vector<glm::vec4> result;
            std::vector<std::vector<float>> float_vec = {};
            float_vec.resize(4);
            GetElementFloat<4>(ply, names, float_vec);
            for (int i = 0; i < float_vec[0].size(); ++i) {
                result.emplace_back(glm::vec4(float_vec[0][i], float_vec[1][i],
                    float_vec[2][i], float_vec[3][i]));
            }
            return result;
        }

    }  // namespace

    Ply::Ply(const std::filesystem::path& file_name) {
        logger_->info("Opening file: {}", file_name.string());
        happly::PLYData ply_in(file_name.string());
        vertices_ = GetElementVertexPropertyVec3(ply_in, { "x", "y", "z" });
        colors_ = GetElementVertexPropertyVec3(ply_in, { "r", "g", "b" });
        if (colors_.empty()) {
            colors_ = GetElementVertexPropertyVec3(
                ply_in,
                { "red", "green", "blue" });
        }
        normals_ = GetElementVertexPropertyVec3(ply_in, { "nx", "ny", "nz" });
        texture_coordinates_ = GetElementVertexPropertyVec2(
            ply_in,
            { "u", "v" });
        if (texture_coordinates_.empty()) {
            texture_coordinates_ = GetElementVertexPropertyVec2(
                ply_in,
                { "s", "t" });
        }
        // Try to get the faces from the file.
        try {
            std::vector<std::vector<std::size_t>> indices =
                ply_in.getFaceIndices();
            for (const auto& element : indices) {
                for (const auto& value : element) {
                    indices_.push_back(static_cast<std::uint32_t>(value));
                }
            }
        }
        catch (const std::exception& e) {
            logger_->warn(e.what());
        }
        // Create a fake indices from 0 to the length of vertices in case there
        // is no indices.
        if (indices_.empty()) {
            indices_.resize(vertices_.size());
            std::iota(indices_.begin(), indices_.end(), 0);
        }
    }

}  // End namespace frame::file.
