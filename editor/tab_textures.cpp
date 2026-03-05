#include "tab_textures.h"
#include "frame/entity_id.h"
#include "frame/gui/window_message_box.h"
#include "frame/gui/window_file_dialog.h"
#include "frame/logger.h"
#include "frame/material_interface.h"
#include "frame/program_interface.h"
#include <algorithm>
#include <stdexcept>
#include <string>

#include <imgui.h>

namespace frame::gui
{

void TabTextures::Draw(LevelInterface& level)
{
    if (selected_texture_id_ != frame::NullId &&
        !HasTextureId(level, selected_texture_id_))
    {
        selected_texture_id_ = frame::NullId;
    }

    const float button_size = ImGui::GetFrameHeight();
    bool open = ImGui::CollapsingHeader(
        "Textures",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
    ImVec2 header_min = ImGui::GetItemRectMin();
    ImVec2 header_max = ImGui::GetItemRectMax();
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorScreenPos(
        {header_max.x - 2.f * button_size - 8.f, header_min.y});
    if (ImGui::Button("-##texture", ImVec2(button_size, button_size)))
    {
        RemoveSelectedTexture(level);
    }
    ImGui::SetCursorScreenPos({header_max.x - button_size - 4.f, header_min.y});
    if (ImGui::Button("+##texture", ImVec2(button_size, button_size)))
    {
        ImGui::OpenPopup("##add_texture_popup");
    }
    if (ImGui::BeginPopup("##add_texture_popup"))
    {
        if (ImGui::Selectable("Import 2D Texture"))
        {
            ShowImportTextureDialog(false);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Import Cubemap (equirect/HDR)"))
        {
            ShowImportTextureDialog(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (open)
    {
        for (auto id : level.GetTextures())
        {
            auto& tex = level.GetTextureFromId(id);
            bool selected = (id == selected_texture_id_);
            if (ImGui::Selectable(tex.GetName().c_str(), selected))
            {
                selected_texture_id_ = id;
            }
            if (ImGui::BeginDragDropSource())
            {
                EntityId payload = id;
                ImGui::SetDragDropPayload(
                    "FRAME_ASSET_ID", &payload, sizeof(payload));
                ImGui::Text("%s", tex.GetName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

void TabTextures::AddTextureFromFile(
    const std::string& file, bool as_cubemap)
{
    try
    {
        if (file.empty())
        {
            throw std::runtime_error("No texture file selected.");
        }
        if (!import_texture_callback_)
        {
            throw std::runtime_error(
                "Texture import callback is not configured.");
        }
        import_texture_callback_(file, as_cubemap);
    }
    catch (const std::exception& e)
    {
        frame::Logger::GetInstance()->error(e.what());
        draw_gui_.AddModalWindow(
            std::make_unique<WindowMessageBox>(
                "Texture import failed",
                e.what()));
    }
}

void TabTextures::ShowImportTextureDialog(bool as_cubemap)
{
    draw_gui_.AddModalWindow(
        std::make_unique<WindowFileDialog>(
            "",
            FileDialogEnum::OPEN,
            [this, as_cubemap](const std::string& file) {
                AddTextureFromFile(file, as_cubemap);
            }));
}

void TabTextures::ResetSelection()
{
    selected_texture_id_ = frame::NullId;
}

bool TabTextures::HasTextureId(const LevelInterface& level, EntityId id) const
{
    if (id == frame::NullId)
    {
        return false;
    }
    const auto texture_ids = level.GetTextures();
    return std::find(texture_ids.begin(), texture_ids.end(), id) !=
           texture_ids.end();
}

bool TabTextures::IsTextureUsed(const LevelInterface& level, EntityId id) const
{
    for (auto material_id : level.GetMaterials())
    {
        const frame::MaterialInterface& mat =
            level.GetMaterialFromId(material_id);
        if (mat.HasTextureId(id))
            return true;
    }
    for (auto program_id : level.GetPrograms())
    {
        const frame::ProgramInterface& prog =
            level.GetProgramFromId(program_id);
        auto inputs = prog.GetInputTextureIds();
        if (std::count(inputs.begin(), inputs.end(), id))
            return true;
        auto outputs = prog.GetOutputTextureIds();
        if (std::count(outputs.begin(), outputs.end(), id))
            return true;
    }
    return false;
}

void TabTextures::CloseTextureWindows(const std::string& name)
{
    for (const std::string& window_name : draw_gui_.GetWindowTitles())
    {
        if ((window_name.starts_with("texture - ") ||
             window_name.starts_with("cubemap - ")) &&
            window_name.find(std::string("[") + name + "]") !=
                std::string::npos)
        {
            draw_gui_.DeleteWindow(window_name);
        }
    }
}

void TabTextures::RemoveSelectedTexture(LevelInterface& level)
{
    if (selected_texture_id_ == frame::NullId)
    {
        draw_gui_.AddModalWindow(
            std::make_unique<WindowMessageBox>(
                "Warning", "No texture selected."));
        return;
    }
    if (!HasTextureId(level, selected_texture_id_))
    {
        selected_texture_id_ = frame::NullId;
        draw_gui_.AddModalWindow(
            std::make_unique<WindowMessageBox>(
                "Warning",
                "Selected texture is no longer valid. Please reselect."));
        return;
    }
    std::string name = level.GetTextureFromId(selected_texture_id_).GetName();
    if (IsTextureUsed(level, selected_texture_id_))
    {
        frame::Logger::GetInstance()->warn(
            "Cannot delete a texture that is still used.");
        draw_gui_.AddModalWindow(
            std::make_unique<WindowMessageBox>(
                "Warning", "Cannot delete a texture that is still used."));
        return;
    }
    CloseTextureWindows(name);
    level.ExtractTexture(selected_texture_id_);
    selected_texture_id_ = frame::NullId;
    if (update_json_callback_)
        update_json_callback_();
}

} // namespace frame::gui
