#include "tab_programs.h"
#include "frame/entity_id.h"
#include <imgui.h>

namespace ed = ax::NodeEditor;

namespace frame::gui {

TabPrograms::~TabPrograms()
{
    if (context_)
    {
        ed::DestroyEditor(context_);
        context_ = nullptr;
    }
}

void TabPrograms::Draw(LevelInterface& level)
{
    if (current_program_ == nullptr)
    {
        if (ImGui::CollapsingHeader("Programs", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto id : level.GetPrograms())
            {
                auto& prog = level.GetProgramFromId(id);
                if (ImGui::Selectable(prog.GetName().c_str()))
                {
                    current_program_ = &prog;
                    initialized_ = false;
                    if (context_)
                    {
                        ed::DestroyEditor(context_);
                        context_ = nullptr;
                    }
                }
                if (ImGui::BeginDragDropSource())
                {
                    EntityId payload = id;
                    ImGui::SetDragDropPayload("FRAME_ASSET_ID", &payload, sizeof(payload));
                    ImGui::Text("%s", prog.GetName().c_str());
                    ImGui::EndDragDropSource();
                }
            }
        }
    }
    else
    {
        if (ImGui::Button("Back##program"))
        {
            current_program_ = nullptr;
            initialized_ = false;
            if (context_)
            {
                ed::DestroyEditor(context_);
                context_ = nullptr;
            }
            return;
        }

        if (!context_)
        {
            config_.SettingsFile = nullptr;
            context_ = ed::CreateEditor(&config_);
        }

        ed::SetCurrentEditor(context_);
        ed::Begin("ProgramView");

        int entry_node = 1;
        int program_node = 2;
        int exit_node = 3;

        // Entry node with uniform outputs
        ed::BeginNode(entry_node);
        ImGui::Text("Entry");
        int pin_index = 0;
        for (const auto& uniform_name : current_program_->GetUniformNameList())
        {
            int pin_id = 10 + pin_index;
            ed::BeginPin(pin_id, ed::PinKind::Output);
            ImGui::Text("%s", uniform_name.c_str());
            ed::EndPin();
            ++pin_index;
        }
        ed::EndNode();

        // Program node with uniform inputs and texture outputs
        ed::BeginNode(program_node);
        ImGui::Text("%s", current_program_->GetName().c_str());
        pin_index = 0;
        for (const auto& uniform_name : current_program_->GetUniformNameList())
        {
            int pin_id = 100 + pin_index;
            ed::BeginPin(pin_id, ed::PinKind::Input);
            ImGui::Text("%s", uniform_name.c_str());
            ed::EndPin();
            ++pin_index;
        }
        pin_index = 0;
        for (auto tex_id : current_program_->GetOutputTextureIds())
        {
            int pin_id = 200 + pin_index;
            ed::BeginPin(pin_id, ed::PinKind::Output);
            auto& tex = level.GetTextureFromId(tex_id);
            ImGui::Text("%s", tex.GetName().c_str());
            ed::EndPin();
            ++pin_index;
        }
        ed::EndNode();

        // Exit node with texture inputs
        ed::BeginNode(exit_node);
        ImGui::Text("Exit");
        pin_index = 0;
        for (auto tex_id : current_program_->GetOutputTextureIds())
        {
            int pin_id = 300 + pin_index;
            ed::BeginPin(pin_id, ed::PinKind::Input);
            auto& tex = level.GetTextureFromId(tex_id);
            ImGui::Text("%s", tex.GetName().c_str());
            ed::EndPin();
            ++pin_index;
        }
        ed::EndNode();

        // Links from entry -> program
        pin_index = 0;
        for (const auto& uniform_name : current_program_->GetUniformNameList())
        {
            ed::Link(1000 + pin_index, 10 + pin_index, 100 + pin_index);
            ++pin_index;
        }

        // Links from program -> exit
        pin_index = 0;
        for (auto tex_id : current_program_->GetOutputTextureIds())
        {
            ed::Link(2000 + pin_index, 200 + pin_index, 300 + pin_index);
            ++pin_index;
        }

        if (!initialized_)
        {
            ed::SetNodePosition(entry_node, ImVec2(-250, 0));
            ed::SetNodePosition(program_node, ImVec2(0, 0));
            ed::SetNodePosition(exit_node, ImVec2(250, 0));
            ed::NavigateToContent();
            initialized_ = true;
        }

        ed::End();
        ed::SetCurrentEditor(nullptr);
    }
}

} // namespace frame::gui
