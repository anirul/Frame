#include "frame/opengl/gui/sdl_opengl_draw_gui.h"

#include "frame/device_interface.h"
#include "frame/window_interface.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

namespace frame::opengl::gui {

SDL2OpenGLDrawGui::SDL2OpenGLDrawGui(frame::DeviceInterface* device, frame::WindowInterface* window)
    : window_interface_(window), device_interface_(device) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    // Setup Platform/Renderer back ends
    ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(window_interface_->GetWindowContext()),
                                 device_interface_->GetDeviceContext());
    ImGui_ImplOpenGL3_Init("#version 430 core");
}

SDL2OpenGLDrawGui::~SDL2OpenGLDrawGui() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void SDL2OpenGLDrawGui::Startup(std::pair<std::uint32_t, std::uint32_t> size) {}

bool SDL2OpenGLDrawGui::RunDraw(double dt) {
    bool returned_value = true;
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(static_cast<SDL_Window*>(window_interface_->GetWindowContext()));
    ImGui::NewFrame();
    for (const auto& pair : callbacks_) {
        // Call the callback!
        ImGui::Begin(pair.second->GetName().c_str());
        if (!pair.second->DrawCallback()) returned_value = false;
        ImGui::End();
    }
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return returned_value;
}

void SDL2OpenGLDrawGui::AddWindow(std::unique_ptr<frame::gui::GuiWindowInterface>&& callback) {
    std::string name = callback->GetName();
    if (name.empty()) throw std::runtime_error("Cannot create a sub window without a name!");
    callbacks_.insert({ name, std::move(callback) });
}

std::vector<std::string> SDL2OpenGLDrawGui::GetWindowTitles() const {
    std::vector<std::string> name_list;
    for (const auto& [name, _] : callbacks_) {
        name_list.push_back(name);
    }
    return name_list;
}

void SDL2OpenGLDrawGui::DeleteWindow(const std::string& name) { callbacks_.erase(name); }

bool SDL2OpenGLDrawGui::PollEvent(void* event) {
    auto& io = ImGui::GetIO();
    ImGui_ImplSDL2_ProcessEvent(static_cast<SDL_Event*>(event));
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

}  // End namespace frame::opengl::gui.
