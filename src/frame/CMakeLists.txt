# Frame part.

# Generate protobuf sources first so that headers are available to
# the rest of the project.
add_subdirectory(proto)

add_library(Frame
  STATIC
    # Included from include/frame.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/api.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/bvh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh_cache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/buffer_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/camera_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/device_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/entity_id.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/image_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/input_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/level_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/level.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/light_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/material_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/name_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/node_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/plugin_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/program_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/renderer_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/serialize_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/serialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/static_mesh_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/texture_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/uniform_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/uniform_collection_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/window_factory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/frame/window_interface.h
    # Based in this directory.
    camera.cpp
    level.cpp
    bvh.cpp
    bvh_cache.cpp
    logger.cpp
    node_camera.cpp
    node_camera.h
    node_light.cpp
    node_light.h
    node_matrix.cpp
    node_matrix.h
    node_static_mesh.cpp
    node_static_mesh.h
    uniform.cpp
    uniform.h
    uniform_collection_wrapper.cpp
    uniform_collection_wrapper.h
    window_factory.cpp
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/src/frame/proto
)

target_link_libraries(Frame
  PUBLIC
    absl::base
    absl::flags
    absl::flags_parse
    absl::strings
    SDL3::SDL3
    spdlog::spdlog
    glm::glm
    FrameProto
)

add_dependencies(Frame FrameProto)

set_property(TARGET Frame PROPERTY FOLDER "Frame")

add_subdirectory(opengl)
add_subdirectory(vulkan)

add_subdirectory(common)
add_subdirectory(file)
add_subdirectory(gui)
add_subdirectory(json)
