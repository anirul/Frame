# Frame part.

# Generate protobuf sources first so that headers are available to
# the rest of the project.
add_subdirectory(proto)

add_library(Frame
  STATIC
    api.h
    buffer_interface.h
    bvh.cpp
    bvh.h
    bvh_cache.cpp
    bvh_cache.h
    camera.cpp
    camera.h
    camera_interface.h
    device_interface.h
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
    name_interface.h
    node_camera.cpp
    node_camera.h
    node_interface.h
    node_light.cpp
    node_light.h
    node_matrix.cpp
    node_matrix.h
    node_static_mesh.cpp
    node_static_mesh.h
    plugin_interface.h
    program_interface.h
    renderer_interface.h
    serialize.h
    serialize_interface.h
    static_mesh_interface.h
    texture_interface.h
    uniform.cpp
    uniform.h
    uniform_interface.h
    uniform_collection_interface.h
    uniform_collection_wrapper.cpp
    uniform_collection_wrapper.h
    window_factory.cpp
    window_factory.h
    window_interface.h
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/frame/proto
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
