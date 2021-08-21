# Frame OpenGL File Test

add_executable(FrameOpenGLFileTest
    main.cpp
    LoadProgramTest.cpp
    LoadProgramTest.h
    LoadStaticMeshTest.cpp
    LoadStaticMeshTest.h
    LoadTextureTest.cpp
    LoadTextureTest.h
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
    FrameOpenGL
    FrameOpenGLFile
    GTest::gmock
    GTest::gtest
)

set_property(TARGET FrameOpenGLFileTest PROPERTY FOLDER "Frame/OpenGL")
