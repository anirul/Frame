#include "tab_textures.h"
#include "frame/entity_id.h"
#include "frame/file/file_system.h"
#include "frame/opengl/texture.h"

#include <imgui.h>

namespace frame::gui
{

void TabTextures::Draw(LevelInterface& level)
{
    const float button_size = ImGui::GetFrameHeight();
    bool open = ImGui::CollapsingHeader(
        "Textures",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
    ImVec2 header_min = ImGui::GetItemRectMin();
    ImVec2 header_max = ImGui::GetItemRectMax();
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorScreenPos(
        {header_max.x - 2.f * button_size - 8.f, header_min.y});
    if (ImGui::Button("-", ImVec2(button_size, button_size)))
    {
        RemoveSelectedTexture(level);
    }
    ImGui::SetCursorScreenPos({header_max.x - button_size - 4.f, header_min.y});
    if (ImGui::Button("+", ImVec2(button_size, button_size)))
    {
        draw_gui_.AddModalWindow(
            std::make_unique<WindowFileDialog>(
                "",
                FileDialogEnum::OPEN,
                [this, &level](const std::string& file) {
                    AddTextureFromFile(level, file);
                }));
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
    LevelInterface& level, const std::string& file)
{
    try
    {
        auto texture = std::make_unique<opengl::Texture>(file);
        std::filesystem::path path = file;
        std::string base_name = path.stem().string();
        std::string name = base_name;
        int counter = 1;
        while (level.GetIdFromName(name) != frame::NullId)
        {
            name = base_name + "_" + std::to_string(counter++);
        }
        texture->SetName(name);
        texture->SetSerializeEnable(true);
        level.AddTexture(std::move(texture));
        if (update_json_callback_)
            update_json_callback_();
    }
    catch (const std::exception&)
    {
        // Ignore errors when loading texture
    }
}

void TabTextures::RemoveSelectedTexture(LevelInterface& level)
{
    if (selected_texture_id_ == frame::NullId)
        return;
    level.ExtractTexture(selected_texture_id_);
    selected_texture_id_ = frame::NullId;
    if (update_json_callback_)
        update_json_callback_();
}

} // namespace frame::gui
