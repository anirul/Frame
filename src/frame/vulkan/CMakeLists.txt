# Frame Vulkan

add_library(FrameVulkan
  STATIC
    debug_callback.cpp
    debug_callback.h
    device.h
    device.cpp
    build_level.cpp
    sdl_vulkan_none.h
    sdl_vulkan_none.cpp
    sdl_vulkan_window.h
    sdl_vulkan_window.cpp
    dispatcher.cpp
    window_factory.h
    window_factory.cpp
)

target_include_directories(FrameVulkan
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/src/frame/proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_link_libraries(FrameVulkan
  PUBLIC
    Frame
    FrameFile
    FrameJson
    Vulkan::Vulkan
    shaderc::shaderc_combined
)

set_property(TARGET FrameVulkan PROPERTY FOLDER "Frame/Vulkan")
