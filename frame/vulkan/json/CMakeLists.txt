# Frame Vulkan Json

set(FRAME_VULKAN_JSON_SOURCES
  parse_level.h
  parse_material.cpp
  parse_material.h
  parse_program.cpp
  parse_program.h
  parse_scene_tree.cpp
  parse_scene_tree.h
  parse_texture.cpp
  parse_texture.h
)

add_library(FrameVulkanJson STATIC ${FRAME_VULKAN_JSON_SOURCES})

target_include_directories(FrameVulkanJson
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameVulkanJson
  PUBLIC
    Frame
    FrameJson
    Vulkan::Vulkan
)

set_property(TARGET FrameVulkanJson PROPERTY FOLDER "Frame/Vulkan")
