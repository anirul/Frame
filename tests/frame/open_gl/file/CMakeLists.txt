# Frame OpenGL File Test

add_executable(FrameOpenGLFileTest
    load_program_test.cpp
    load_program_test.h
    load_static_mesh_test.cpp
    load_static_mesh_test.h
    load_texture_test.cpp
    load_texture_test.h
    main.cpp
)

target_include_directories(FrameOpenGLFileTest
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    FrameOpenGLFile
)

target_link_libraries(FrameOpenGLFileTest
  PUBLIC
    Frame
    FrameFile
    FrameOpenGL
    FrameOpenGLFile
    GTest::gmock
    GTest::gtest
)

set_property(TARGET FrameOpenGLFileTest PROPERTY FOLDER "Frame/OpenGL")
