#include "frame/gui/window_shader_file.h"

#include <fstream>
#include <cmath>
#include <imgui.h>
#include <format>

namespace frame::gui {

WindowShaderFile::WindowShaderFile(const std::string& file_name)
    : name_(std::format("Shader File [{}]", file_name)), file_name_(file_name) {
    editor_.SetLanguageDefinition(TextEditor::LanguageDefinitionId::Glsl);
    try {
        std::ifstream file(frame::file::FindFile(file_name_));
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            editor_.SetText(content);
        }
    } catch (...) {
        // ignore missing file
    }
}

bool WindowShaderFile::DrawCallback() {
    if (ImGui::Button("Save")) {
        try {
            std::ofstream out(frame::file::FindFile(file_name_));
            out << editor_.GetText();
            error_message_.clear();
        } catch (const std::exception& e) {
            error_message_ = e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        end_ = true;
    }
    ImGui::Separator();
    if (!error_message_.empty()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float text_height =
            ImGui::CalcTextSize(error_message_.c_str(), nullptr, false, avail.x).y;
        float padding = ImGui::GetStyle().WindowPadding.y;
        text_height = std::ceil(text_height + padding * 2);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        ImGui::BeginChild("##error_message",
                         ImVec2(0, text_height),
                         true,
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextWrapped("%s", error_message_.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
    }
    ImVec2 avail = ImGui::GetContentRegionAvail();
    editor_.Render("##shadertext", true, avail, false);
    return true;
}

bool WindowShaderFile::End() const {
    return end_;
}

std::string WindowShaderFile::GetName() const {
    return name_;
}

void WindowShaderFile::SetName(const std::string& name) {
    name_ = name;
}

} // End namespace frame::gui.
