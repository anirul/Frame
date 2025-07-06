#include "window_level.h"

#include <imgui.h>

namespace frame::gui
{

WindowLevel::WindowLevel(DeviceInterface& device) : device_(device)
{
}

void WindowLevel::DisplayNode(LevelInterface& level, EntityId id)
{
    auto& node = level.GetSceneNodeFromId(id);
    std::string name = level.GetNameFromId(id);
    auto children = level.GetChildList(id);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool open = ImGui::TreeNodeEx(
        (void*)(static_cast<intptr_t>(id)), flags, "%s", name.c_str());
    if (open && !children.empty())
    {
        for (auto child : children)
        {
            DisplayNode(level, child);
        }
        ImGui::TreePop();
    }
}

bool WindowLevel::DrawCallback()
{
    auto& level = device_.GetLevel();
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
            auto root = level.GetDefaultRootSceneNodeId();
            if (root)
            {
                DisplayNode(level, root);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
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
