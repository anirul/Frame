# Frame Vulkan

add_library(FrameVulkan
  STATIC
    debug_callback.cpp
    debug_callback.h
    device.h
    device.cpp
    sdl_vulkan_none.h
    sdl_vulkan_none.cpp
    sdl_vulkan_window.h
    sdl_vulkan_window.cpp
    window_factory.h
    window_factory.cpp
)

target_include_directories(FrameVulkan
PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameVulkan
  PUBLIC
    Frame
    FrameFile
    FrameJson
    Vulkan::Vulkan
)

set_property(TARGET FrameVulkan PROPERTY FOLDER "Frame/Vulkan")
