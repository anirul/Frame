#include "frame/opengl/gui/sdl_opengl_draw_gui.h"

#include <SDL3/SDL.h>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/opengl/texture.h"
#include "frame/window_interface.h"

namespace frame::opengl::gui
{

SDLOpenGLDrawGui::SDLOpenGLDrawGui(
    frame::WindowInterface& window,
    const std::filesystem::path& font_path,
    float font_size)
    : window_(window), device_(window_.GetDevice()), font_path_(font_path),
      font_size_(font_size)
{
    // Set the name this is a way to find the plugin.
    SetName("DrawGuiInterface");
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Enable Multi-Viewport
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // / Platform Windows
    ImGuiBackendFlags_PlatformHasViewports;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

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

    // Setup Platform/Renderer back ends
    ImGui_ImplSDL3_InitForOpenGL(
        static_cast<SDL_Window*>(window_.GetWindowContext()),
        device_.GetDeviceContext());
    ImGui_ImplOpenGL3_Init("#version 330");
}

SDLOpenGLDrawGui::~SDLOpenGLDrawGui()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void SDLOpenGLDrawGui::Startup(glm::uvec2 size)
{
    size_ = size;
}

bool SDLOpenGLDrawGui::Update(DeviceInterface& device, double dt)
{
    // Local variables.
    bool returned_value = true;
    is_keyboard_passed_ = false;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
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
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        // Make the other window visible.
        for (const auto& pair : window_callbacks_)
        {
            // Call the callback!
            ImGui::Begin(pair.second.callback->GetName().c_str());
            if (!pair.second.callback->DrawCallback())
                returned_value = false;
            ImGui::End();
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

    // Go through all texture and create a window for the main output.
    for (const EntityId& id : device.GetLevel().GetTextures())
    {
        frame::TextureInterface& texture_interface =
            device.GetLevel().GetTextureFromId(id);
        if (texture_interface.GetData().cubemap())
        {
            continue;
        }
        opengl::Texture& texture = dynamic_cast<opengl::Texture&>(
            device.GetLevel().GetTextureFromId(id));
        auto& level = device.GetLevel();
        bool is_default_output = level.GetIdFromName(texture.GetName()) ==
                                 level.GetDefaultOutputTextureId();
        // This is not the default window so skip.
        if (!is_default_output)
        {
            continue;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        original_image_size_ = texture.GetSize();

        if (!is_visible_)
        {
            ImGui::Begin(
                std::format(
                    "<fullscreen> - [{}] - ({}, {})",
                    texture.GetName(),
                    texture.GetSize().x,
                    texture.GetSize().y)
                    .c_str(),
                nullptr,
                ImGuiWindowFlags_NoDecoration);
        }
        else
        {
            ImGui::Begin(
                std::format(
                    "default - [{}] - ({}, {})",
                    texture.GetName(),
                    texture.GetSize().x,
                    texture.GetSize().y)
                    .c_str());
        }
        if (modal_callback_)
        {
            if (!start_modal_)
            {
                ImGui::OpenPopup(modal_callback_->GetName().c_str());
                start_modal_ = true;
            }
            if (ImGui::BeginPopupModal(
                    modal_callback_->GetName().c_str(),
                    nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize |
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
        // Check if you should enable default window keyboard and mouse.
        if (ImGui::IsWindowHovered())
        {
            is_keyboard_passed_ = true;
        }
        // Get the window width.
        ImVec2 content_window = ImGui::GetContentRegionAvail();
        auto size = texture.GetSize();
        // Compute the aspect ratio.
        float aspect_ratio =
            static_cast<float>(size.x) / static_cast<float>(size.y);
        // Cast the opengl windows id.
        ImTextureID gl_id = static_cast<ImTextureID>(texture.GetId());
        // Compute the final size.
        ImVec2 window_range{};
        if (content_window.x / aspect_ratio > content_window.y)
        {
            window_range =
                ImVec2(content_window.y * aspect_ratio, content_window.y);
        }
        else
        {
            window_range =
                ImVec2(content_window.x, content_window.x / aspect_ratio);
        }
        // Draw the image.
        ImGui::Image(gl_id, window_range, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
        if (is_default_output)
        {
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
            // Setup overlay window with appropriate flags
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

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    io.DisplaySize =
        ImVec2(static_cast<float>(size_.x), static_cast<float>(size_.y));

    SDL_Window* backup_window =
        static_cast<SDL_Window*>(window_.GetWindowContext());
    SDL_GLContext backup_context =
        static_cast<SDL_GLContext>(window_.GetGraphicContext());

    assert(backup_window);
    assert(backup_context);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    SDL_GL_MakeCurrent(backup_window, backup_context);

    return returned_value;
}

void SDLOpenGLDrawGui::AddWindow(
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
    window_callbacks_.emplace(name, std::move(callback_data));
}

void SDLOpenGLDrawGui::AddOverlayWindow(
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
    overlay_callbacks_.emplace(name, std::move(callback_data));
}

void SDLOpenGLDrawGui::AddModalWindow(
    std::unique_ptr<frame::gui::GuiWindowInterface> callback)
{
    modal_callback_ = std::move(callback);
}

std::vector<std::string> SDLOpenGLDrawGui::GetWindowTitles() const
{
    std::vector<std::string> name_list;
    for (const auto& [name, _] : window_callbacks_)
    {
        name_list.push_back(name);
    }
    return name_list;
}

void SDLOpenGLDrawGui::DeleteWindow(const std::string& name)
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

void SDLOpenGLDrawGui::SetMenuBar(
    std::unique_ptr<frame::gui::GuiMenuBarInterface> callback)
{
    menubar_callback_ = std::move(callback);
}

frame::gui::GuiMenuBarInterface& SDLOpenGLDrawGui::GetMenuBar()
{
    return *menubar_callback_.get();
}

void SDLOpenGLDrawGui::RemoveMenuBar()
{
    menubar_callback_.reset();
}

bool SDLOpenGLDrawGui::PollEvent(void* event)
{
    // Allow to click on close window.
    if (static_cast<SDL_Event*>(event)->type == SDL_EVENT_QUIT)
        return false;
    // Process the event in ImGui (has to be done or you won't be able to
    // handle the main window).
    ImGui_ImplSDL3_ProcessEvent(static_cast<SDL_Event*>(event));
    // This is the main window (receiving the input for) so skip.
    if (is_keyboard_passed_)
        return false;
    auto& io = ImGui::GetIO();
    return (!is_keyboard_passed_locked_)
               ? io.WantCaptureMouse || io.WantCaptureKeyboard
               : false;
}

frame::gui::GuiWindowInterface& SDLOpenGLDrawGui::GetWindow(
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

} // End namespace frame::opengl::gui.
