#include "frame/vulkan/gui/sdl_vulkan_draw_gui.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <utility>

#include "frame/logger.h"
#include "frame/vulkan/swapchain_resources.h"
#include "frame/vulkan/texture.h"

namespace frame::vulkan::gui
{

namespace
{

constexpr std::uint32_t kDescriptorPoolSize = 1000;
constexpr std::uint32_t kDefaultMinImageCount = 2;
constexpr bool kShowDefaultOutputPreviewWhenDockVisible = false;

} // namespace

SDLVulkanDrawGui::SDLVulkanDrawGui(
    frame::WindowInterface& window,
    const std::filesystem::path& font_path,
    float font_size)
    : font_path_(font_path),
      window_(window),
      vulkan_device_([&window]() -> frame::vulkan::Device& {
          auto* device = dynamic_cast<frame::vulkan::Device*>(&window.GetDevice());
          if (!device)
          {
              throw std::runtime_error("SDLVulkanDrawGui requires a Vulkan device.");
          }
          return *device;
      }()),
      device_(window_.GetDevice()),
      font_size_(font_size)
{
    SetName("DrawGuiInterface");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;

    if (font_path_.empty())
    {
        io.Fonts->AddFontDefault();
    }
    else
    {
        io.Fonts->AddFontFromFileTTF(
            reinterpret_cast<const char*>(font_path_.u8string().c_str()),
            font_size_);
    }

    ImGui_ImplSDL3_InitForVulkan(
        static_cast<SDL_Window*>(window_.GetWindowContext()));

    vulkan_device_.SetGuiRenderCallback(
        [this](vk::CommandBuffer command_buffer) {
            RenderDrawData(command_buffer);
        });
}

SDLVulkanDrawGui::~SDLVulkanDrawGui()
{
    vulkan_device_.ClearGuiRenderCallback();
    ShutdownRendererBackend();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void SDLVulkanDrawGui::CheckVkResult(VkResult err)
{
    if (err != VK_SUCCESS)
    {
        frame::Logger::GetInstance()->error(
            "ImGui Vulkan backend error: {}",
            vk::to_string(static_cast<vk::Result>(err)));
    }
}

void SDLVulkanDrawGui::EnsureRendererBackend()
{
    auto vk_device = vulkan_device_.GetVkDevice();
    const auto* swapchain = vulkan_device_.GetSwapchainResources();
    if (!vk_device || !swapchain)
    {
        return;
    }
    if (!swapchain->IsValid())
    {
        return;
    }

    const VkRenderPass render_pass =
        static_cast<VkRenderPass>(swapchain->GetRenderPass().get());
    const std::uint32_t image_count =
        static_cast<std::uint32_t>(swapchain->GetImages().size());
    if (render_pass == VK_NULL_HANDLE || image_count == 0)
    {
        return;
    }

    if (renderer_initialized_ && renderer_render_pass_ == render_pass &&
        renderer_image_count_ == image_count)
    {
        return;
    }

    if (renderer_initialized_)
    {
        ShutdownRendererBackend();
    }

    std::array<vk::DescriptorPoolSize, 11> pool_sizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eCombinedImageSampler, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eSampledImage, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eStorageImage, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformTexelBuffer, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eStorageTexelBuffer, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eStorageBuffer, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBufferDynamic, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eStorageBufferDynamic, kDescriptorPoolSize),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eInputAttachment, kDescriptorPoolSize)};

    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        kDescriptorPoolSize * static_cast<std::uint32_t>(pool_sizes.size()),
        static_cast<std::uint32_t>(pool_sizes.size()),
        pool_sizes.data());
    descriptor_pool_ = vk_device.createDescriptorPoolUnique(pool_info);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = static_cast<VkInstance>(vulkan_device_.GetVkInstance());
    init_info.PhysicalDevice =
        static_cast<VkPhysicalDevice>(vulkan_device_.GetVkPhysicalDevice());
    init_info.Device = static_cast<VkDevice>(vk_device);
    init_info.QueueFamily = vulkan_device_.GetGraphicsQueueFamilyIndex();
    init_info.Queue = static_cast<VkQueue>(vulkan_device_.GetGraphicsQueue());
    init_info.RenderPass = render_pass;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = static_cast<VkDescriptorPool>(descriptor_pool_.get());
    init_info.Subpass = 0;
    init_info.MinImageCount = std::max(kDefaultMinImageCount, image_count);
    init_info.ImageCount = image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = CheckVkResult;

    if (!ImGui_ImplVulkan_Init(&init_info))
    {
        throw std::runtime_error("Failed to initialize ImGui Vulkan backend.");
    }
    if (!ImGui_ImplVulkan_CreateFontsTexture())
    {
        throw std::runtime_error("Failed to upload ImGui Vulkan font texture.");
    }

    renderer_initialized_ = true;
    renderer_render_pass_ = render_pass;
    renderer_image_count_ = image_count;
}

void SDLVulkanDrawGui::ShutdownRendererBackend()
{
    if (!renderer_initialized_)
    {
        descriptor_pool_.reset();
        renderer_render_pass_ = VK_NULL_HANDLE;
        renderer_image_count_ = 0;
        return;
    }

    auto vk_device = vulkan_device_.GetVkDevice();
    if (vk_device)
    {
        vk_device.waitIdle();
    }

    ClearTextureBindings();
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();
    descriptor_pool_.reset();
    renderer_initialized_ = false;
    renderer_render_pass_ = VK_NULL_HANDLE;
    renderer_image_count_ = 0;
}

void SDLVulkanDrawGui::RenderDrawData(vk::CommandBuffer command_buffer)
{
    if (!renderer_initialized_)
    {
        return;
    }
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (!draw_data || draw_data->CmdListsCount == 0)
    {
        return;
    }
    ImGui_ImplVulkan_RenderDrawData(
        draw_data,
        static_cast<VkCommandBuffer>(command_buffer));
}

void SDLVulkanDrawGui::Startup(glm::uvec2 size)
{
    size_ = size;
}

bool SDLVulkanDrawGui::Update(DeviceInterface& device, double dt)
{
    bool returned_value = true;
    is_keyboard_passed_ = false;

    auto& level = device.GetLevel();
    if (last_level_ != &level)
    {
        ClearTextureBindings();
        last_level_ = &level;
    }

    EnsureRendererBackend();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    if (renderer_initialized_)
    {
        ImGui_ImplVulkan_NewFrame();
    }
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (!is_visible_)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
    }
    else
    {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode);
        std::vector<std::string> windows_to_remove;
        for (auto& [name, data] : window_callbacks_)
        {
            ImGui::Begin(data.callback->GetName().c_str(), &data.open);
            if (!data.callback->DrawCallback())
            {
                returned_value = false;
            }
            ImGui::End();
            if (data.callback->End() || !data.open)
            {
                windows_to_remove.push_back(name);
            }
        }
        for (const auto& name : windows_to_remove)
        {
            DeleteWindow(name);
        }
    }

    if (menubar_callback_ && is_visible_)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (!menubar_callback_->DrawCallback())
            {
                returned_value = false;
            }
            ImGui::EndMainMenuBar();
        }
    }

    const bool draw_default_output_texture =
        !is_visible_ || kShowDefaultOutputPreviewWhenDockVisible;
    if (draw_default_output_texture)
    {
        auto default_texture_id = level.GetDefaultOutputTextureId();
        for (const EntityId& id : level.GetTextures())
        {
            frame::TextureInterface& texture_interface = level.GetTextureFromId(id);
            if (texture_interface.GetData().cubemap())
            {
                continue;
            }
            auto* texture = dynamic_cast<frame::vulkan::Texture*>(&texture_interface);
            if (!texture || !texture->HasGpuResources())
            {
                continue;
            }
            if (id != default_texture_id)
            {
                continue;
            }

            VkDescriptorSet imgui_texture = GetOrCreateTextureId(id, *texture);
            if (imgui_texture == VK_NULL_HANDLE)
            {
                continue;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            original_image_size_ = texture->GetSize();

            if (!is_visible_)
            {
                ImGui::Begin(
                    std::format(
                        "<fullscreen> - [{}] - ({}, {})",
                        texture->GetName(),
                        texture->GetSize().x,
                        texture->GetSize().y)
                        .c_str(),
                    nullptr,
                    ImGuiWindowFlags_NoDecoration);
            }
            else
            {
                ImGui::Begin(
                    std::format(
                        "default - [{}] - ({}, {})",
                        texture->GetName(),
                        texture->GetSize().x,
                        texture->GetSize().y)
                        .c_str());
            }

            if (modal_callback_)
            {
                if (!start_modal_)
                {
                    glm::vec2 modal_size = modal_callback_->GetInitialSize();
                    if (modal_size.x > 0.f || modal_size.y > 0.f)
                    {
                        ImGui::SetNextWindowSize(
                            ImVec2(modal_size.x, modal_size.y),
                            ImGuiCond_Appearing);
                    }
                    ImGui::OpenPopup(modal_callback_->GetName().c_str());
                    start_modal_ = true;
                }
                if (ImGui::BeginPopupModal(
                        modal_callback_->GetName().c_str(),
                        nullptr,
                        ImGuiWindowFlags_NoMove))
                {
                    modal_callback_->DrawCallback();
                    if (modal_callback_->End())
                    {
                        start_modal_ = false;
                        ImGui::CloseCurrentPopup();
                        modal_callback_.reset();
                    }
                    ImGui::EndPopup();
                }
            }

            if (ImGui::IsWindowHovered())
            {
                is_keyboard_passed_ = true;
            }

            ImVec2 content_window = ImGui::GetContentRegionAvail();
            auto texture_size = texture->GetSize();
            if (texture_size.y == 0)
            {
                ImGui::End();
                ImGui::PopStyleVar();
                continue;
            }
            float aspect_ratio = static_cast<float>(texture_size.x) /
                                 static_cast<float>(texture_size.y);
            ImVec2 image_size{};
            if (content_window.x / aspect_ratio > content_window.y)
            {
                image_size = ImVec2(content_window.y * aspect_ratio, content_window.y);
            }
            else
            {
                image_size = ImVec2(content_window.x, content_window.x / aspect_ratio);
            }
            ImGui::Image(reinterpret_cast<ImTextureID>(imgui_texture), image_size);
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }

    if (!is_visible_)
    {
        for (const auto& pair : overlay_callbacks_)
        {
            ImGui::SetNextWindowPos(ImVec2(
                static_cast<float>(pair.second.position.x),
                static_cast<float>(pair.second.position.y)));
            ImGui::SetNextWindowSize(ImVec2(
                static_cast<float>(pair.second.size.x),
                static_cast<float>(pair.second.size.y)));
            ImGui::Begin(
                pair.first.c_str(),
                nullptr,
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
            pair.second.callback->DrawCallback();
            ImGui::End();
        }
    }

    ImGui::Render();
    io.DisplaySize = ImVec2(static_cast<float>(size_.x), static_cast<float>(size_.y));

    return returned_value;
}

void SDLVulkanDrawGui::AddWindow(
    std::unique_ptr<frame::gui::GuiWindowInterface> callback)
{
    std::string name = callback->GetName();
    if (name.empty())
    {
        throw std::runtime_error("Cannot create a sub window without a name!");
    }
    CallbackData callback_data;
    callback_data.callback = std::move(callback);
    callback_data.position = glm::vec2(0.0f, 0.0f);
    callback_data.size = glm::vec2(0.0f, 0.0f);
    callback_data.open = true;
    window_callbacks_.emplace(name, std::move(callback_data));
}

void SDLVulkanDrawGui::AddOverlayWindow(
    glm::vec2 position,
    glm::vec2 size,
    std::unique_ptr<frame::gui::GuiWindowInterface> callback)
{
    std::string name = callback->GetName();
    if (name.empty())
    {
        throw std::runtime_error("Cannot create a sub window without a name!");
    }
    CallbackData callback_data;
    callback_data.callback = std::move(callback);
    callback_data.position = position;
    callback_data.size = size;
    callback_data.open = true;
    overlay_callbacks_.emplace(name, std::move(callback_data));
}

void SDLVulkanDrawGui::AddModalWindow(
    std::unique_ptr<frame::gui::GuiWindowInterface> callback)
{
    modal_callback_ = std::move(callback);
    start_modal_ = false;
}

std::vector<std::string> SDLVulkanDrawGui::GetWindowTitles() const
{
    std::vector<std::string> name_list;
    for (const auto& [name, _] : window_callbacks_)
    {
        name_list.push_back(name);
    }
    return name_list;
}

void SDLVulkanDrawGui::DeleteWindow(const std::string& name)
{
    if (window_callbacks_.contains(name))
    {
        window_callbacks_.erase(name);
    }
    if (overlay_callbacks_.contains(name))
    {
        overlay_callbacks_.erase(name);
    }
}

void SDLVulkanDrawGui::SetMenuBar(
    std::unique_ptr<frame::gui::GuiMenuBarInterface> callback)
{
    menubar_callback_ = std::move(callback);
}

frame::gui::GuiMenuBarInterface& SDLVulkanDrawGui::GetMenuBar()
{
    return *menubar_callback_.get();
}

void SDLVulkanDrawGui::RemoveMenuBar()
{
    menubar_callback_.reset();
}

bool SDLVulkanDrawGui::PollEvent(void* event)
{
    SDL_Event* sdl_event = static_cast<SDL_Event*>(event);
    ImGui_ImplSDL3_ProcessEvent(sdl_event);
    if (sdl_event->type == SDL_EVENT_QUIT)
    {
        return false;
    }
    auto& io = ImGui::GetIO();
    return (!is_keyboard_passed_locked_)
               ? io.WantCaptureMouse || io.WantCaptureKeyboard
               : false;
}

frame::gui::GuiWindowInterface& SDLVulkanDrawGui::GetWindow(
    const std::string& name)
{
    if (window_callbacks_.contains(name))
    {
        return *window_callbacks_.at(name).callback.get();
    }
    if (overlay_callbacks_.contains(name))
    {
        return *overlay_callbacks_.at(name).callback.get();
    }
    throw std::runtime_error("Cannot find the window with the name: " + name);
}

VkDescriptorSet SDLVulkanDrawGui::GetOrCreateTextureId(
    EntityId texture_id,
    frame::vulkan::Texture& texture)
{
    if (!renderer_initialized_)
    {
        return VK_NULL_HANDLE;
    }
    if (auto it = texture_bindings_.find(texture_id); it != texture_bindings_.end())
    {
        return it->second;
    }

    auto descriptor_info = texture.GetDescriptorInfo();
    VkDescriptorSet descriptor_set = ImGui_ImplVulkan_AddTexture(
        static_cast<VkSampler>(descriptor_info.sampler),
        static_cast<VkImageView>(descriptor_info.imageView),
        static_cast<VkImageLayout>(descriptor_info.imageLayout));
    texture_bindings_.emplace(texture_id, descriptor_set);
    return descriptor_set;
}

void SDLVulkanDrawGui::ClearTextureBindings()
{
    if (renderer_initialized_)
    {
        for (const auto& [_, descriptor_set] : texture_bindings_)
        {
            if (descriptor_set != VK_NULL_HANDLE)
            {
                ImGui_ImplVulkan_RemoveTexture(descriptor_set);
            }
        }
    }
    texture_bindings_.clear();
}

} // namespace frame::vulkan::gui
