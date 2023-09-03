#include "frame/gui/window_cubemap.h"

#include <imgui.h>

#include <filesystem>

#include "frame/file/file_system.h"
#include "frame/level_interface.h"

namespace frame::gui {

    WindowCubemap::WindowCubemap(const std::string& name) : name_(name) {}

    bool WindowCubemap::DrawCallback() {
        ImGui::Text("Select cubemap: ");
        std::vector<std::filesystem::path> cubemaps = GetCubemaps();
        for (auto& cubemap : cubemaps) {
            if (ImGui::Button(cubemap.filename().string().c_str())) {
                SaveCubemapPath(cubemap);
                end_ = false;
                return false;
            }
        }
        end_ = true;
        return true;
    }

    std::string WindowCubemap::GetName() const { return name_; }

    void WindowCubemap::SetName(const std::string& name) { name_ = name; }

    bool WindowCubemap::End() const { return end_; }

    std::vector<std::filesystem::path> WindowCubemap::GetCubemaps() {
        std::vector<std::filesystem::path> matching_files;
        auto directory = file::FindDirectory("asset/cubemap");
        std::string extension = ".hdr";
        if (std::filesystem::exists(directory) &&
            std::filesystem::is_directory(directory)) {
            for (const auto& entry :
                std::filesystem::directory_iterator(directory))
            {
                if (entry.is_regular_file() && entry.path().extension() ==
                    extension)
                {
                    matching_files.push_back(entry.path());
                }
            }
        }
        return matching_files;
    }

    void WindowCubemap::SaveCubemapPath(const std::filesystem::path& path) {
        cubemap_path_ = path;
    }

    void WindowCubemap::ChangeLevel(frame::proto::Level& level) {
        if (!cubemap_path_.empty()) {
            for (auto& texture : *level.mutable_textures()) {
                if (texture.name() == "skybox") {
                    texture.set_file_name(cubemap_path_.string());
                }
            }
        }
    }

}  // End namespace frame::gui.
