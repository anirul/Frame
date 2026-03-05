#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <unordered_map>

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/window_json_file.h"
#include "frame/json/proto.h"

#include "tab_materials.h"
#include "tab_programs.h"
#include "tab_scene.h"
#include "tab_textures.h"

namespace frame::gui
{

class WindowLevel : public WindowJsonFile
{
  public:
    WindowLevel(
        DeviceInterface& device,
        DrawGuiInterface& draw_gui,
        const std::string& file_name);
    ~WindowLevel() override = default;

    /** @brief Update the JSON editor with the current level state. */
    void UpdateJsonEditor();

    bool DrawCallback() override;

  protected:
    void ApplyJsonContent(const std::string& content) override;

  private:
    enum class ViewMode
    {
        Scene = 0,
        Json = 1,
        Advanced = 2,
    };

    void ShowImportGltfDialog();
    void ShowImportCubemapDialog();
    void ImportGltfSceneFromFile(const std::string& file_name);
    void ImportTextureFromFile(
        const std::string& file_name, bool as_cubemap);
    void SetSkyboxFromCubemapName(const std::string& cubemap_name);
    void CaptureCubemapSourceSnapshots(const frame::proto::Level& proto_level);
    bool AddSkyboxNode();
    void ApplyProtoLevel(const frame::proto::Level& proto_level);

    DeviceInterface& device_;
    DrawGuiInterface& draw_gui_;
    TabTextures tab_textures_;
    TabPrograms tab_programs_;
    TabMaterials tab_materials_;
    TabScene tab_scene_;
    ViewMode view_mode_ = ViewMode::Scene;
    std::string selected_skybox_cubemap_name_ = {};
    std::unordered_map<std::string, frame::proto::Texture>
        cubemap_source_snapshots_ = {};
};

} // namespace frame::gui
