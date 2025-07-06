#include "window_level.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstdint>
#include <imgui-node-editor/imgui_node_editor.h>
#include <imgui.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/entity_id.h"

namespace ed = ax::NodeEditor;

namespace frame::gui
{

WindowLevel::WindowLevel(DeviceInterface& device, const std::string& file_name)
    : device_(device), file_name_(file_name)
{
    editor_.SetLanguageDefinition(TextEditor::LanguageDefinitionId::Json);
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
        // ignore
    }
}

WindowLevel::~WindowLevel()
{
    if (context_)
        ed::DestroyEditor(context_);
}

void WindowLevel::DisplayNode(LevelInterface& level, EntityId id, EntityId parent)
{
    auto name = level.GetNameFromId(id);
    ed::BeginNode(id);

    auto input_id = id * 2 + 1;
    auto output_id = id * 2 + 2;

    ed::BeginPin(input_id, ed::PinKind::Input);
    ImGui::Dummy(ImVec2(10, 10));
    ed::EndPin();

    ImGui::SameLine();
    ImGui::Text("%s", name.c_str());
    ImGui::SameLine();

    ed::BeginPin(output_id, ed::PinKind::Output);
    ImGui::Dummy(ImVec2(10, 10));
    ed::EndPin();

    ed::EndNode();

    if (parent != frame::NullId)
    {
        auto parent_output = parent * 2 + 2;
        std::uint64_t link_id = (static_cast<std::uint64_t>(parent) << 32) |
            static_cast<std::uint64_t>(id);
        ed::Link(static_cast<ed::LinkId>(link_id), parent_output, input_id);
    }

    for (auto child : level.GetChildList(id))
    {
        DisplayNode(level, child, id);
    }
}

bool WindowLevel::DrawCallback()
{
    auto& level = device_.GetLevel();
    auto draw_toggle = [&]() {
        if (show_json_)
        {
            ImGui::BeginDisabled();
            ImGui::Button("JSON Editor");
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Node Editor"))
                show_json_ = false;
        }
        else
        {
            if (ImGui::Button("JSON Editor"))
                show_json_ = true;
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Node Editor");
            ImGui::EndDisabled();
        }
    };

    if (show_json_)
    {
        draw_toggle();
        ImGui::SameLine();
        if (ImGui::Button("Apply"))
        {
            try
            {
                std::string content = editor_.GetText();
                auto new_level =
                    frame::json::ParseLevel(device_.GetSize(), content);
                device_.Startup(std::move(new_level));
                error_message_.clear();
            }
            catch (const std::exception& e)
            {
                error_message_ = e.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load"))
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
                ImGui::CalcTextSize(
                    error_message_.c_str(), nullptr, false, avail.x)
                    .y;
            float padding = ImGui::GetStyle().WindowPadding.y;
            text_height = std::ceil(text_height + padding * 2);
            ImGui::PushStyleColor(
                ImGuiCol_ChildBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            ImGui::BeginChild(
                "##error_message",
                ImVec2(0, text_height),
                true,
                ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
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
            editor_.Render("##jsontext", focused, avail, false);
        }
        else
        {
            ImGui::BeginDisabled();
            editor_.Render("##jsontext", focused, avail, false);
            ImGui::EndDisabled();
        }
    }
    else
    {
        if (!context_)
            context_ = ed::CreateEditor();
        draw_toggle();
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            // TODO: implement saving level
        }
        ImGui::Separator();
        if (ImGui::BeginTabBar("##level_tabs"))
        {
            if (ImGui::BeginTabItem("Textures"))
            {
                for (auto id : level.GetTextures())
                {
                    auto& tex = level.GetTextureFromId(id);
                    ImGui::BulletText("%s", tex.GetName().c_str());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Programs"))
            {
                for (auto id : level.GetPrograms())
                {
                    auto& prog = level.GetProgramFromId(id);
                    ImGui::BulletText("%s", prog.GetName().c_str());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Materials"))
            {
                for (auto id : level.GetMaterials())
                {
                    auto& mat = level.GetMaterialFromId(id);
                    ImGui::BulletText("%s", mat.GetName().c_str());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Scene"))
            {
                ed::SetCurrentEditor(context_);
                ed::Begin("SceneEditor");
                auto root = level.GetDefaultRootSceneNodeId();
                if (root)
                {
                    DisplayNode(level, root, frame::NullId);
                }
                ed::End();
                ed::SetCurrentEditor(nullptr);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    return true;
}

bool WindowLevel::End() const
{
    return end_;
}

std::string WindowLevel::GetName() const
{
    return name_;
}

void WindowLevel::SetName(const std::string& name)
{
    name_ = name;
}

} // namespace frame::gui
