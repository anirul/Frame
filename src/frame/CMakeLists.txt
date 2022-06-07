# Frame part.

add_library(Frame
  STATIC
    buffer_interface.h
    camera.cpp
    camera.h
    device_interface.h
    draw_interface.h
    entity_id.h
    image_interface.h
    input_interface.h
    level.cpp
    level.h
    level_interface.h
    light_interface.h
    logger.cpp
    logger.h
    material_interface.h
    program_interface.h
    name_interface.h
    node_camera.cpp
    node_camera.h
    node_light.cpp
    node_light.h
    node_matrix.cpp
    node_matrix.h
    node_interface.h
    node_static_mesh.cpp
    node_static_mesh.h
    renderer_interface.h
    static_mesh_interface.h
    texture_interface.h
    uniform_interface.h
    uniform_wrapper.cpp
    uniform_wrapper.h
    window.cpp
    window.h
    window_interface.h
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_SOURCE_DIR}
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
