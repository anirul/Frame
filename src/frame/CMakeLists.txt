# Frame part.

add_library(Frame
  STATIC
    # Included form include/frame.
    ${CMAKE_SOURCE_DIR}/include/frame/buffer_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/camera.h
    ${CMAKE_SOURCE_DIR}/include/frame/device_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/draw_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/entity_id.h
    ${CMAKE_SOURCE_DIR}/include/frame/image_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/input_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/level_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/light_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/material_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/name_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/node_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/program_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/renderer_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/static_mesh_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/texture_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/uniform_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/window_interface.h
    ${CMAKE_SOURCE_DIR}/include/frame/window.h
    # Based in this directory.
    camera.cpp
    level.cpp
    level.h
    logger.cpp
    logger.h
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
    window.cpp
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Frame
  PUBLIC
    FrameProto
    SDL2::SDL2
    spdlog::spdlog
    glm::glm
)

set_property(TARGET Frame PROPERTY FOLDER "Frame")

add_subdirectory(file)
add_subdirectory(open_gl)
add_subdirectory(proto)
