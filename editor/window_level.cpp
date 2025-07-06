#include "window_level.h"

#include <imgui.h>
#include <imgui-node-editor/imgui_node_editor.h>

namespace ed = ax::NodeEditor;

namespace frame::gui
{

WindowLevel::WindowLevel(DeviceInterface& device) : device_(device)
{
}

WindowLevel::~WindowLevel()
{
    if (context_)
        ed::DestroyEditor(context_);
}

void WindowLevel::DisplayNode(LevelInterface& level, EntityId id)
{
    std::string name = level.GetNameFromId(id);
    ed::BeginNode(id);
    ImGui::Text("%s", name.c_str());
    ed::EndNode();
    for (auto child : level.GetChildList(id))
    {
        DisplayNode(level, child);
    }
}

bool WindowLevel::DrawCallback()
{
    auto& level = device_.GetLevel();
    if (!context_)
        context_ = ed::CreateEditor();
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
                DisplayNode(level, root);
            }
            ed::End();
            ed::SetCurrentEditor(nullptr);
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
