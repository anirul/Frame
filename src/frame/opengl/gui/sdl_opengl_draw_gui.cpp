#include "frame/opengl/gui/sdl_opengl_draw_gui.h"

#include <SDL2/SDL.h>
#include <fmt/core.h>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/opengl/texture.h"
#include "frame/window_interface.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

namespace frame::opengl::gui
{

SDL2OpenGLDrawGui::SDL2OpenGLDrawGui(frame::WindowInterface &window)
    : window_(window), device_(window_.GetDevice())
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
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

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    io.Fonts->AddFontFromFileTTF(
        file::FindFile("asset/font/poppins/Poppins-Light.ttf").string().c_str(),
        20.0f);

    // Setup Platform/Renderer back ends
    ImGui_ImplSDL2_InitForOpenGL(
        static_cast<SDL_Window *>(window_.GetWindowContext()),
        device_.GetDeviceContext());
    ImGui_ImplOpenGL3_Init("#version 450");
}

SDL2OpenGLDrawGui::~SDL2OpenGLDrawGui()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void SDL2OpenGLDrawGui::Startup(glm::uvec2 size)
{
    size_ = size;
}

bool SDL2OpenGLDrawGui::Update(DeviceInterface &device, double dt)
{
    // Local variables.
    bool returned_value = true;
    is_keyboard_passed_ = false;
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(
        static_cast<SDL_Window *>(window_.GetWindowContext()));
    ImGui::NewFrame();

    if (!is_visible_)
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
    }
    else
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        // Make the other window visible.
        for (const auto &pair : callbacks_)
        {
            // Call the callback!
            ImGui::Begin(pair.second->GetName().c_str());
            if (!pair.second->DrawCallback())
                returned_value = false;
            ImGui::End();
        }
    }

    // Go through all texture and create a window for each of them.
    for (const EntityId &id : device.GetLevel().GetAllTextures())
    {
        frame::TextureInterface &texture_interface =
            device.GetLevel().GetTextureFromId(id);
        if (texture_interface.IsCubeMap())
            continue;
        opengl::Texture &texture = dynamic_cast<opengl::Texture &>(
            device.GetLevel().GetTextureFromId(id));
        auto &level = device.GetLevel();
        bool is_default_output = level.GetIdFromName(texture.GetName()) ==
                                 level.GetDefaultOutputTextureId();
        if (is_default_output)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        }
        // If the window are not visible and it is not the main window then
        // bail out.
        if (!is_visible_ && !is_default_output)
            continue;
        if (!is_visible_ && is_default_output)
        {
            ImGui::Begin(
                fmt::format("{} - <fullscreen>", texture.GetName()).c_str(),
                nullptr,
                ImGuiWindowFlags_NoDecoration);
        }
        else
        {
            ImGui::Begin(texture.GetName().c_str());
        }
        // Check if you should enable default window keyboard and mouse.
        if (ImGui::IsWindowHovered() && is_default_output)
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
        // I disable the warning C4312 from unsigned int to void* casting to
        // a bigger space.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4312)
#endif
        ImTextureID gl_id = reinterpret_cast<ImTextureID>(texture.GetId());
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif
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

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    io.DisplaySize =
        ImVec2(static_cast<float>(size_.x), static_cast<float>(size_.y));

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(
            static_cast<SDL_Window *>(window_.GetWindowContext()),
            window_.GetGraphicContext());
    }

    return returned_value;
}

void SDL2OpenGLDrawGui::AddWindow(
    std::unique_ptr<frame::gui::GuiWindowInterface> &&callback)
{
    std::string name = callback->GetName();
    if (name.empty())
    {
        throw std::runtime_error("Cannot create a sub window without a name!");
    }
    callbacks_.insert({name, std::move(callback)});
}

std::vector<std::string> SDL2OpenGLDrawGui::GetWindowTitles() const
{
    std::vector<std::string> name_list;
    for (const auto &[name, _] : callbacks_)
    {
        name_list.push_back(name);
    }
    return name_list;
}

void SDL2OpenGLDrawGui::DeleteWindow(const std::string &name)
{
    callbacks_.erase(name);
}

bool SDL2OpenGLDrawGui::PollEvent(void *event)
{
    // Allow to click on close window.
    if (static_cast<SDL_Event *>(event)->type == SDL_QUIT)
        return false;
    // Process the event in ImGui (has to be done or you won't be able to
    // handle the main window).
    ImGui_ImplSDL2_ProcessEvent(static_cast<SDL_Event *>(event));
    // This is the main window (receiving the input for) so skip.
    if (is_keyboard_passed_)
        return false;
    auto &io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

frame::gui::GuiWindowInterface &SDL2OpenGLDrawGui::GetWindow(
    const std::string &name)
{
    return *callbacks_.at(name).get();
}

} // End namespace frame::opengl::gui.
