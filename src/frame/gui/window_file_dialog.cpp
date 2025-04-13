#include "frame/gui/window_file_dialog.h"

#include <format>
#include <imgui.h>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <algorithm>

namespace frame::gui
{

// Constructor: Initialize the dialog with a file extension filter and mode.
WindowFileDialog::WindowFileDialog(
    const std::string& extension,
    FileDialogEnum file_dialog_enum,
    std::function<void(const std::string&)> get_file)
    : name_("File Dialog"),
      extension_(extension),
      file_dialog_enum_(file_dialog_enum),
      get_file_(get_file)
{
    switch (file_dialog_enum_)
    {
    case FileDialogEnum::NEW:
        file_dialog_enum_ = FileDialogEnum::NEW;
        name_ = std::format("New File [{}]", extension_);
        break;
    case FileDialogEnum::OPEN:
        file_dialog_enum_ = FileDialogEnum::OPEN;
        name_ = std::format("Open File [{}]", extension_);
        break;
    case FileDialogEnum::SAVE_AS:
        file_dialog_enum_ = FileDialogEnum::SAVE_AS;
        name_ = std::format("Save File [{}]", extension_);
        break;
    }
}

bool WindowFileDialog::DrawCallback()
{
    // Ensure the window does not grow larger than desired.
    ImGuiIO& io = ImGui::GetIO();
    // Define a minimum window size and a maximum (80% of display size).
    ImVec2 minSize(400, 300);
    ImVec2 maxSize(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f);
    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

    // Static buffer to hold the selected file name/path.
    static char file_buffer[256] = "";
    namespace fs = std::filesystem;
    static fs::path current_path = fs::current_path();

    // Display the file extension filter at the top.
    ImGui::Text("Filter: %s", extension_.c_str());

    // Calculate the reserved bottom space for the input text and button.
    float bottom_offset = ImGui::GetFrameHeightWithSpacing() * 2.0f;

    // Compute the available height for the child region.
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float computed_child_height = avail.y - bottom_offset;
    // Clamp the file browser's height so it doesn't force the window to grow
    // too tall.
    float maxChildHeight =
        300.0f; // maximum desired height for the file browser area
    float child_height = std::min(computed_child_height, maxChildHeight);
    if (child_height < 100.f)
        child_height = 100.f; // enforce a minimum height

    // Begin a child region for file browsing with the clamped height.
    if (ImGui::BeginChild("FileBrowser", ImVec2(0, child_height), true))
    {
        // Display the current directory.
        ImGui::Text("Directory: %s", current_path.string().c_str());

        // Allow navigation to the parent directory.
        if (current_path.has_parent_path())
        {
            if (ImGui::Selectable(".."))
            {
                current_path = current_path.parent_path();
            }
        }

        // Compute an effective extension (ensuring it starts with a dot).
        std::string effective_extension = extension_;
        if (!effective_extension.empty() && effective_extension[0] != '.')
            effective_extension = "." + effective_extension;

        // Iterate over entries in the current directory.
        for (const auto& entry : fs::directory_iterator(current_path))
        {
            // Apply extension filtering for regular files.
            if (!extension_.empty() && entry.is_regular_file())
            {
                if (entry.path().extension() != effective_extension)
                    continue;
            }

            // Prepare the display label, marking directories appropriately.
            std::string label = entry.path().filename().string();
            if (entry.is_directory())
                label = "[DIR] " + label;

            if (ImGui::Selectable(label.c_str()))
            {
                if (entry.is_directory())
                {
                    current_path = entry.path();
                }
                else
                {
                    // Copy the selected file's full path into the file_buffer
                    // using std::copy_n.
                    std::string fullPath = entry.path().string();
                    size_t len =
                        std::min(fullPath.size(), sizeof(file_buffer) - 1);
                    std::copy_n(fullPath.begin(), len, file_buffer);
                    file_buffer[len] = '\0';
                }
            }
        }
    }
    ImGui::EndChild();

    // Separator between the file browser and the bottom controls.
    ImGui::Separator();

    // If we're in NEW mode, and the file_buffer doesn't already start with the
    // current directory...
    if (file_dialog_enum_ == FileDialogEnum::NEW)
    {
        std::string current_dir = current_path.string();
        std::string current_buffer(file_buffer);
        // Check if file_buffer already contains the current directory (use
        // find() == 0 to check prefix).
        if (current_buffer.empty() || current_buffer.find(current_dir) != 0)
        {
            // For example, if file_buffer is empty, prefill it with the full
            // path plus a file separator.
            std::string new_full_path = current_dir;
            if (new_full_path.back() != '/' && new_full_path.back() != '\\')
            {
#if defined(_WIN32) || defined(_WIN64)
                new_full_path.push_back('\\');
#else
                new_full_path.push_back('/');
#endif
            }
            // If the user typed something (e.g. a file name without path),
            // append it.
            new_full_path += current_buffer;
            // Copy back to file_buffer (using std::copy_n for safety).
            size_t len =
                std::min(new_full_path.size(), sizeof(file_buffer) - 1);
            std::copy_n(new_full_path.begin(), len, file_buffer);
            file_buffer[len] = '\0';
        }
    }

    // Input field for the file name/path.
    ImGui::InputText("File Name", file_buffer, sizeof(file_buffer));

    switch (file_dialog_enum_)
    {
    case FileDialogEnum::NEW:
        if (ImGui::Button("New File"))
        {
            file_name_ =
                (std::strlen(file_buffer)) ? file_buffer : file_name_;
            get_file_(file_name_);
            end_ = true;
        }
        break;
    case FileDialogEnum::OPEN:
        if (ImGui::Button("Open File"))
        {
            file_name_ =
                (std::strlen(file_buffer)) ? file_buffer : file_name_;
            get_file_(file_name_);
            end_ = true;
        }
        break;
    case FileDialogEnum::SAVE_AS:
        if (ImGui::Button("Save As"))
        {
            file_name_ =
                (std::strlen(file_buffer)) ? file_buffer : file_name_;
            get_file_(file_name_);
            end_ = true;
        }
        break;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        end_ = true;
    }

    return true;
}

std::string WindowFileDialog::GetName() const
{
    return name_;
}

void WindowFileDialog::SetName(const std::string& name)
{
    name_ = name;
}

bool WindowFileDialog::End() const
{
    return end_;
}

} // End namespace frame::gui.
