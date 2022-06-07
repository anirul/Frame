# Frame OpenGL Test.

add_executable(FrameOpenGLTest
    buffer_test.cpp
    buffer_test.h
    device_test.cpp
    device_test.h
    frame_buffer_test.cpp
    frame_buffer_test.h
    light_test.cpp
    light_test.h
    main.cpp
    material_test.cpp
    material_test.h
    static_mesh_test.cpp
    static_mesh_test.h
    pixel_test.cpp
    pixel_test.h
    program_test.cpp
    program_test.h
    render_buffer_test.cpp
    render_buffer_test.h
    renderer_test.cpp
    renderer_test.h
    shader_test.cpp
    shader_test.h
    texture_cube_map_test.cpp
    texture_cube_map_test.h
    texture_test.cpp
    texture_test.h
    ${CMAKE_SOURCE_DIR}/asset/json/device_test.json
    ${CMAKE_SOURCE_DIR}/asset/json/renderer_test.json
)

target_include_directories(FrameOpenGLTest
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    FrameOpenGL
)

target_link_libraries(FrameOpenGLTest
  PUBLIC
    Frame
    FrameFile
    FrameOpenGL
    FrameOpenGLFile
    FrameProto
    GTest::gmock
    GTest::gtest
)

set_property(TARGET FrameOpenGLTest PROPERTY FOLDER "Frame/OpenGL")

add_subdirectory(file)
