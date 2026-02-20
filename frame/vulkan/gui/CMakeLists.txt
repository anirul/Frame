# Frame Vulkan Gui.

add_library(FrameVulkanGui
  STATIC
    sdl_vulkan_draw_gui.cpp
    sdl_vulkan_draw_gui.h
)

target_include_directories(FrameVulkanGui
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameVulkanGui
  PRIVATE
    Frame
    FrameGui
    FrameVulkan
    Vulkan::Vulkan
    imgui::imgui
    SDL3::SDL3
    spdlog::spdlog
)

set_property(TARGET FrameVulkanGui PROPERTY FOLDER "Frame/Vulkan")
