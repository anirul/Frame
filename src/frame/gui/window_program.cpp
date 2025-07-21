#include "frame/gui/window_program.h"

#include <format>
#include <imgui.h>

namespace ed = ax::NodeEditor;

namespace frame::gui {

WindowProgram::WindowProgram(LevelInterface& level, ProgramInterface& program)
    : level_(level), program_(program) {
    name_ = std::format("program - [{}]", program_.GetName());
}

WindowProgram::~WindowProgram() {
    if (context_)
        ed::DestroyEditor(context_);
}

bool WindowProgram::DrawCallback() {
    if (!context_)
        context_ = ed::CreateEditor();

    ed::SetCurrentEditor(context_);
    ed::Begin("ProgramEditor");

    int program_node = 1;
    ed::BeginNode(program_node);
    ImGui::Text("%s", program_.GetName().c_str());

    int pin_index = 0;
    for (auto tex_id : program_.GetInputTextureIds()) {
        int pin_id = 10 + pin_index;
        ed::BeginPin(pin_id, ed::PinKind::Input);
        ImGui::Text("in%d", pin_index);
        ed::EndPin();
        ++pin_index;
    }
    pin_index = 0;
    for (auto tex_id : program_.GetOutputTextureIds()) {
        int pin_id = 20 + pin_index;
        ed::BeginPin(pin_id, ed::PinKind::Output);
        ImGui::Text("out%d", pin_index);
        ed::EndPin();
        ++pin_index;
    }
    ed::EndNode();

    int index = 0;
    pin_index = 0;
    for (auto tex_id : program_.GetInputTextureIds()) {
        int node_id = 100 + index;
        int tex_pin = 1000 + index;
        auto& tex = level_.GetTextureFromId(tex_id);
        ed::BeginNode(node_id);
        ImGui::Text("%s", tex.GetName().c_str());
        ed::BeginPin(tex_pin, ed::PinKind::Output);
        ImGui::Dummy(ImVec2(10,10));
        ed::EndPin();
        ed::EndNode();
        ed::Link(10000 + index, tex_pin, 10 + pin_index);
        ++index;
        ++pin_index;
    }

    index = 0;
    pin_index = 0;
    for (auto tex_id : program_.GetOutputTextureIds()) {
        int node_id = 200 + index;
        int tex_pin = 2000 + index;
        auto& tex = level_.GetTextureFromId(tex_id);
        ed::BeginNode(node_id);
        ImGui::Text("%s", tex.GetName().c_str());
        ed::BeginPin(tex_pin, ed::PinKind::Input);
        ImGui::Dummy(ImVec2(10,10));
        ed::EndPin();
        ed::EndNode();
        ed::Link(20000 + index, 20 + pin_index, tex_pin);
        ++index;
        ++pin_index;
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);
    return true;
}

bool WindowProgram::End() const { return false; }

std::string WindowProgram::GetName() const { return name_; }

void WindowProgram::SetName(const std::string& name) { name_ = name; }

} // namespace frame::gui

