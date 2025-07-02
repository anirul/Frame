#include "frame/gui/window_glsl_file.h"

#include "frame/logger.h"
#include <cmath>
#include <format>
#include <fstream>
#include <imgui.h>
#include <string_view>

namespace frame::gui
{

WindowGlslFile::WindowGlslFile(
    const std::string& file_name, DeviceInterface& device)
    : name_(std::format("GLSL File [{}]", file_name)), file_name_(file_name),
      device_(device)
{
    editor_.SetLanguageDefinition(TextEditor::LanguageDefinitionId::Glsl);
    try
    {
        std::ifstream file(frame::file::FindFile(file_name_));
        if (file)
        {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            editor_.SetText(content);
        }
    }
    catch (...)
    {
        // ignore missing file
    }
}

bool WindowGlslFile::DrawCallback()
{
    if (ImGui::Button("Compile"))
    {
        try
        {
            std::string source = editor_.GetText();
            std::ofstream out(frame::file::FindFile(file_name_));
            out << source;

            using frame::opengl::Shader;
            using frame::opengl::ShaderEnum;
            ShaderEnum shader_type = ShaderEnum::FRAGMENT_SHADER;
            if (std::string_view(file_name_).ends_with(".vert"))
                shader_type = ShaderEnum::VERTEX_SHADER;
            Shader shader(shader_type);
            if (!shader.LoadFromSource(source))
            {
                throw std::runtime_error(shader.GetErrorMessage());
            }
            device_.Resize(device_.GetSize());
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
            frame::Logger::GetInstance()->error(e.what());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        try
        {
            std::ifstream file(frame::file::FindFile(file_name_));
            if (file)
            {
                std::string content(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
                editor_.SetText(content);
            }
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        try
        {
            std::ofstream out(frame::file::FindFile(file_name_));
            out << editor_.GetText();
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
        }
    }
    ImGui::Separator();
    if (!error_message_.empty())
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float text_height =
            ImGui::CalcTextSize(error_message_.c_str(), nullptr, false, avail.x)
                .y;
        float padding = ImGui::GetStyle().WindowPadding.y;
        text_height = std::ceil(text_height + padding * 2);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        ImGui::BeginChild(
            "##error_message",
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
    bool focused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (focused)
    {
        editor_.Render("##shadertext", focused, avail, false);
    }
    else
    {
        ImGui::BeginDisabled();
        editor_.Render("##shadertext", focused, avail, false);
        ImGui::EndDisabled();
    }
    return true;
}

bool WindowGlslFile::End() const
{
    return end_;
}

std::string WindowGlslFile::GetName() const
{
    return name_;
}

void WindowGlslFile::SetName(const std::string& name)
{
    name_ = name;
}

} // End namespace frame::gui.
