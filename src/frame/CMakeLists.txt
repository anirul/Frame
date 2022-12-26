# Frame part.

add_library(Frame
  STATIC
    # Included from include/frame.
    ${CMAKE_SOURCE_DIR}/include/frame/api.h
    ${CMAKE_SOURCE_DIR}/include/frame/buffer_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/camera.h
    ${CMAKE_SOURCE_DIR}/include/frame/context.h
    ${CMAKE_SOURCE_DIR}/include/frame/device_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/entity_id.h
    ${CMAKE_SOURCE_DIR}/include/frame/image_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/input_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/level_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/level.h
    ${CMAKE_SOURCE_DIR}/include/frame/light_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/logger.h
    ${CMAKE_SOURCE_DIR}/include/frame/material_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/name_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/node_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/plugin_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/program_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/renderer_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/static_mesh_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/texture_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/uniform_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/window_factory.h
    ${CMAKE_SOURCE_DIR}/include/frame/window_interface.h
    # Based in this directory.
    camera.cpp
    level.cpp
    logger.cpp
    node_camera.cpp
    node_camera.h
    node_light.cpp
    node_light.h
    node_matrix.cpp
    node_matrix.h
    node_static_mesh.cpp
    node_static_mesh.h
    uniform_wrapper.cpp
    uniform_wrapper.h
    window_factory.cpp
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Frame
  PUBLIC
    absl::base
    absl::flags
    absl::flags_parse
    absl::strings
    FrameJson
    FrameOpenGL
    FrameVulkan
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    spdlog::spdlog
    glm::glm
)

set_property(TARGET Frame PROPERTY FOLDER "Frame")

add_subdirectory(opengl)
add_subdirectory(vulkan)

add_subdirectory(common)
add_subdirectory(file)
add_subdirectory(gui)
add_subdirectory(json)
add_subdirectory(proto)
