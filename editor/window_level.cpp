#include "window_level.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <imgui-node-editor/imgui_node_editor.h>
#include <imgui.h>

#include "frame/entity_id.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace ed = ax::NodeEditor;

namespace frame::gui
{

WindowLevel::WindowLevel(DeviceInterface& device, const std::string& file_name)
    : WindowJsonFile(file_name, device), device_(device)
{
}

WindowLevel::~WindowLevel()
{
    if (context_)
        ed::DestroyEditor(context_);
}

void WindowLevel::DisplayNode(
    LevelInterface& level, EntityId id, EntityId parent)
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

void WindowLevel::DrawTexturesTab(LevelInterface& level)
{
    for (auto id : level.GetTextures())
    {
        auto& tex = level.GetTextureFromId(id);
        ImGui::BulletText("%s", tex.GetName().c_str());
    }
}

void WindowLevel::DrawProgramsTab(LevelInterface& level)
{
    for (auto id : level.GetPrograms())
    {
        auto& prog = level.GetProgramFromId(id);
        ImGui::BulletText("%s", prog.GetName().c_str());
    }
}

void WindowLevel::DrawMaterialsTab(LevelInterface& level)
{
    for (auto id : level.GetMaterials())
    {
        auto& mat = level.GetMaterialFromId(id);
        ImGui::BulletText("%s", mat.GetName().c_str());
    }
}

void WindowLevel::DrawSceneTab(LevelInterface& level)
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
        WindowJsonFile::DrawCallback();
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
                DrawTexturesTab(level);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Programs"))
            {
                DrawProgramsTab(level);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Materials"))
            {
                DrawMaterialsTab(level);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Scene"))
            {
                DrawSceneTab(level);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    return true;
}

} // namespace frame::gui
