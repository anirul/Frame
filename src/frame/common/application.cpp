#include "frame/common/application.h"

#include <glm/gtc/matrix_transform.hpp>

#include "frame/common/draw.h"

namespace frame::common {

Application::Application(std::unique_ptr<frame::WindowInterface>&& window)
    : window_(std::move(window)) {
    assert(window_);
}

void Application::Startup(std::filesystem::path path) {
    assert(window_);
    if (index_ != -1) {
        window_->RemoveDrawInterface(index_);
    }
    index_ = window_->AddDrawInterface(
        std::make_unique<Draw>(window_->GetSize(), path, window_->GetUniqueDevice()));
}

void Application::Startup(std::unique_ptr<frame::LevelInterface>&& level) {
    assert(window_);
    if (index_ != -1) {
        window_->RemoveDrawInterface(index_);
    }
    index_ = window_->AddDrawInterface(
        std::make_unique<Draw>(window_->GetSize(), std::move(level), window_->GetUniqueDevice()));
}

void Application::Resize(std::pair<std::uint32_t, std::uint32_t> size) {
    assert(window_);
    window_->Resize(size);
}

void Application::Run() {
    assert(window_);
    window_->Run();
}

void Application::SetFullscreen(frame::FullScreenEnum fullscreen_mode) {
    assert(window_);
    window_->SetFullScreen(fullscreen_mode);
}

}  // End namespace frame::common.
