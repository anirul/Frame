#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/gui/gui_menu_bar_interface.h"
#include "frame/uniform_collection_interface.h"
#include "frame/vulkan/device.h"
#include "frame/vulkan/vulkan_dispatch.h"
#include "frame/window_interface.h"

namespace frame::vulkan::gui
{

/**
 * @class SDLVulkanDrawGui
 * @brief Draw GUI elements using SDL and Vulkan.
 */
class SDLVulkanDrawGui : public frame::gui::DrawGuiInterface
{
  public:
    SDLVulkanDrawGui(
        frame::WindowInterface& window,
        const std::filesystem::path& font_path,
        float font_size);
    ~SDLVulkanDrawGui() override;

  public:
    std::string GetName() const override
    {
        return name_;
    }
    void SetName(const std::string& name) override
    {
        name_ = name;
    }
    void PreRender(
        UniformCollectionInterface& uniform,
        DeviceInterface& device,
        StaticMeshInterface& static_mesh,
        MaterialInterface& material) override
    {
    }
    void End() override
    {
    }
    void SetVisible(bool enable) override
    {
        is_visible_ = enable;
    }
    bool IsVisible() const override
    {
        return is_visible_;
    }
    void SetKeyboardPassed(bool is_keyboard_passed) override
    {
        is_keyboard_passed_locked_ = is_keyboard_passed;
    }
    bool IsKeyboardPassed() const override
    {
        return is_keyboard_passed_locked_;
    }
    DeviceInterface& GetDevice() override
    {
        return device_;
    }

  public:
    void AddWindow(
        std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    void AddOverlayWindow(
        glm::vec2 position,
        glm::vec2 size,
        std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    void AddModalWindow(
        std::unique_ptr<frame::gui::GuiWindowInterface> callback) override;
    frame::gui::GuiWindowInterface& GetWindow(
        const std::string& name) override;
    std::vector<std::string> GetWindowTitles() const override;
    void DeleteWindow(const std::string& name) override;
    void SetMenuBar(
        std::unique_ptr<frame::gui::GuiMenuBarInterface> callback) override;
    frame::gui::GuiMenuBarInterface& GetMenuBar() override;
    void RemoveMenuBar() override;
    void Startup(glm::uvec2 size) override;
    bool Update(DeviceInterface& device, double dt = 0.0) override;
    bool PollEvent(void* event) override;

  private:
    struct CallbackData
    {
        std::unique_ptr<frame::gui::GuiWindowInterface> callback = nullptr;
        glm::vec2 position = {0.0f, 0.0f};
        glm::vec2 size = {0.0f, 0.0f};
        bool open = true;
    };

    static void CheckVkResult(VkResult err);
    void EnsureRendererBackend();
    void ShutdownRendererBackend();
    void RenderDrawData(vk::CommandBuffer command_buffer);
    VkDescriptorSet GetOrCreateTextureId(
        EntityId texture_id,
        const vk::DescriptorImageInfo& descriptor_info);
    VkDescriptorSet GetOrCreateTextureId(
        EntityId texture_id,
        frame::vulkan::Texture& texture);
    void ClearTextureBindings();

  private:
    std::map<std::string, CallbackData> window_callbacks_ = {};
    std::map<std::string, CallbackData> overlay_callbacks_ = {};
    std::unique_ptr<frame::gui::GuiWindowInterface> modal_callback_ = nullptr;
    std::unique_ptr<frame::gui::GuiMenuBarInterface> menubar_callback_ =
        nullptr;
    std::unordered_map<EntityId, VkDescriptorSet> texture_bindings_ = {};
    const frame::LevelInterface* last_level_ = nullptr;
    std::filesystem::path font_path_;
    frame::WindowInterface& window_;
    frame::vulkan::Device& vulkan_device_;
    frame::DeviceInterface& device_;
    std::string name_;
    glm::uvec2 size_ = {0, 0};
    glm::uvec2 original_image_size_ = {0, 0};
    bool start_modal_ = false;
    bool is_keyboard_passed_locked_ = false;
    bool is_keyboard_passed_ = false;
    bool is_visible_ = true;
    float font_size_ = 20.0f;
    vk::UniqueDescriptorPool descriptor_pool_;
    bool renderer_initialized_ = false;
    VkRenderPass renderer_render_pass_ = VK_NULL_HANDLE;
    std::uint32_t renderer_image_count_ = 0;
};

} // namespace frame::vulkan::gui
