# Frame OpenGL.

add_library(FrameOpenGL
  STATIC
    bind_interface.h
    buffer.cpp
    buffer.h
    device.cpp
    device.h
    frame_buffer.cpp
    frame_buffer.h
    light.cpp
    light.h
    material.cpp
    material.h
    static_mesh.cpp
    static_mesh.h
    pixel.cpp
    pixel.h
    message_callback.cpp
    message_callback.h
    program.cpp
    program.h
    render_buffer.cpp
    render_buffer.h
    renderer.cpp
    renderer.h
    scoped_bind.cpp
    scoped_bind.h
    shader.cpp
    shader.h
    texture.cpp
    texture.h
    cubemap.cpp
    cubemap.h
    cubemap_views.h
    sdl_opengl_none.cpp
    sdl_opengl_none.h
    sdl_opengl_window.cpp
    sdl_opengl_window.h
    window_factory.cpp
    window_factory.h
)

target_include_directories(FrameOpenGL
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameOpenGL
  PRIVATE
    absl::base
    absl::flags
    absl::flags_parse
    absl::strings
    Frame
    FrameFile
    FrameJson
    FrameOpenGLFile
    GLEW::GLEW
    glm::glm
    protobuf::libprotobuf
    SDL3::SDL3
    spdlog::spdlog
)

set_property(TARGET FrameOpenGL PROPERTY FOLDER "Frame/OpenGL")

add_subdirectory(file)
add_subdirectory(gui)
