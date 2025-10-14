# Frame Vulkan

set(FRAME_VULKAN_DATA_SOURCES
  buffer.cpp
  buffer.h
  build_level.cpp
  json/parse_material.cpp
  json/parse_program.cpp
  json/parse_scene_tree.cpp
  json/parse_texture.cpp
  material.cpp
  material.h
  program.cpp
  program.h
  static_mesh.cpp
  static_mesh.h
  texture.cpp
  texture.h
)

add_library(FrameVulkanData STATIC ${FRAME_VULKAN_DATA_SOURCES})

target_include_directories(FrameVulkanData
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/src/frame/proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_link_libraries(FrameVulkanData
  PUBLIC
    Frame
    FrameJson
)

set_property(TARGET FrameVulkanData PROPERTY FOLDER "Frame/Vulkan")

add_library(FrameVulkan
  STATIC
    debug_callback.cpp
    debug_callback.h
    device.h
    device.cpp
    dispatcher.cpp
    sdl_vulkan_none.h
    sdl_vulkan_none.cpp
    sdl_vulkan_window.h
    sdl_vulkan_window.cpp
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
    FrameVulkanData
    Frame
    FrameFile
    FrameJson
    Vulkan::Vulkan
    shaderc::shaderc_combined
)

set_property(TARGET FrameVulkan PROPERTY FOLDER "Frame/Vulkan")
