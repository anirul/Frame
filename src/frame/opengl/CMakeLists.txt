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
    texture_cube_map.cpp
    texture_cube_map.h
    sdl_opengl_none.cpp
    sdl_opengl_none.h
    sdl_opengl_window.cpp
    sdl_opengl_window.h
    window.cpp
    window.h
    win32_opengl_none.cpp
    win32_opengl_none.h
)

target_include_directories(FrameOpenGL
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameOpenGL
  PUBLIC
    Frame
    FrameFile
    FrameJson
    FrameOpenGLFile
    GLEW::GLEW
)

set_property(TARGET FrameOpenGL PROPERTY FOLDER "Frame/OpenGL")

add_subdirectory(file)
add_subdirectory(gui)
