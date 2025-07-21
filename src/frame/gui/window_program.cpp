#include "frame/gui/window_program.h"

#include <format>
#include <imgui.h>

namespace ed = ax::NodeEditor;

namespace frame::gui
{

WindowProgram::WindowProgram(LevelInterface& level, ProgramInterface& program)
    : level_(level), program_(program)
{
    name_ = std::format("program - [{}]", program_.GetName());
}

WindowProgram::~WindowProgram()
{
    if (context_)
    {
        ed::DestroyEditor(context_);
        context_ = nullptr;
    }
}

bool WindowProgram::DrawCallback()
{
    if (!context_)
        context_ = ed::CreateEditor();

    ed::SetCurrentEditor(context_);
    ed::Begin(name_.c_str());

    int entry_node = 1;
    int program_node = 2;
    int exit_node = 3;

    // Entry node with uniform outputs
    ed::BeginNode(entry_node);
    ImGui::Text("Entry");
    int pin_index = 0;
    for (const auto& uniform_name : program_.GetUniformNameList())
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
    ImGui::Text("%s", program_.GetName().c_str());
    pin_index = 0;
    for (const auto& uniform_name : program_.GetUniformNameList())
    {
        int pin_id = 100 + pin_index;
        ed::BeginPin(pin_id, ed::PinKind::Input);
        ImGui::Text("%s", uniform_name.c_str());
        ed::EndPin();
        ++pin_index;
    }
    pin_index = 0;
    for (auto tex_id : program_.GetOutputTextureIds())
    {
        int pin_id = 200 + pin_index;
        ed::BeginPin(pin_id, ed::PinKind::Output);
        auto& tex = level_.GetTextureFromId(tex_id);
        ImGui::Text("%s", tex.GetName().c_str());
        ed::EndPin();
        ++pin_index;
    }
    ed::EndNode();

    // Exit node with texture inputs
    ed::BeginNode(exit_node);
    ImGui::Text("Exit");
    pin_index = 0;
    for (auto tex_id : program_.GetOutputTextureIds())
    {
        int pin_id = 300 + pin_index;
        ed::BeginPin(pin_id, ed::PinKind::Input);
        auto& tex = level_.GetTextureFromId(tex_id);
        ImGui::Text("%s", tex.GetName().c_str());
        ed::EndPin();
        ++pin_index;
    }
    ed::EndNode();

    // Links from entry -> program
    pin_index = 0;
    for (const auto& uniform_name : program_.GetUniformNameList())
    {
        ed::Link(1000 + pin_index, 10 + pin_index, 100 + pin_index);
        ++pin_index;
    }

    // Links from program -> exit
    pin_index = 0;
    for (auto tex_id : program_.GetOutputTextureIds())
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
    return true;
}

bool WindowProgram::End() const
{
    return false;
}

std::string WindowProgram::GetName() const
{
    return name_;
}

void WindowProgram::SetName(const std::string& name)
{
    name_ = name;
}

} // namespace frame::gui
