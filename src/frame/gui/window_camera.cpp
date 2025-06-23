#include "frame/gui/window_camera.h"

#include <format>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace frame::gui
{

void WindowCamera::RestoreCamera(float aspect_ratio)
{
    reset_camera_ = *camera_ptr_;
    camera_ptr_->SetAspectRatio(aspect_ratio);
    position_ = camera_ptr_->GetPosition();
    front_ = camera_ptr_->GetFront();
    up_ = camera_ptr_->GetUp();
    fov_degrees_ = camera_ptr_->GetFovDegrees();
    near_clip_ = camera_ptr_->GetNearClip();
    far_clip_ = camera_ptr_->GetFarClip();
}

bool WindowCamera::DrawCallback()
{
    static bool once = true;
    if (std::exchange(once, false))
    {
        auto* viewport = ImGui::GetMainViewport();
        RestoreCamera(viewport->Size.x / viewport->Size.y);
    }
    ImGui::Text(
        "%s",
        std::format("Aspect ratio: {}", camera_ptr_->GetAspectRatio()).c_str());
    ImGui::Separator();
    ImGui::DragFloat3("Position", glm::value_ptr(position_));
    ImGui::DragFloat3("Front", glm::value_ptr(front_));
    ImGui::DragFloat3("Up", glm::value_ptr(up_));
    ImGui::DragFloat("Far clip", &far_clip_);
    ImGui::DragFloat("Near clip", &near_clip_);
    ImGui::DragFloat("Field of view (degrees)", &fov_degrees_);
    ImGui::Separator();
    auto* viewport = ImGui::GetMainViewport();
    float aspect_ratio = viewport->Size.x / viewport->Size.y;
    if (ImGui::Button("Reset"))
    {
        camera_ptr_->operator=(reset_camera_);
        RestoreCamera(aspect_ratio);
    }
    ImGui::SameLine();
    if (ImGui::Button("Get"))
    {
        RestoreCamera(aspect_ratio);
    }
    ImGui::SameLine();
    if (ImGui::Button("Set"))
    {
        camera_ptr_->operator=(frame::Camera(
            position_,
            front_,
            up_,
            fov_degrees_,
            aspect_ratio,
            near_clip_,
            far_clip_));
        RestoreCamera(aspect_ratio);
    }
    end_ = true;
    return true;
}

} // End namespace frame::gui.
