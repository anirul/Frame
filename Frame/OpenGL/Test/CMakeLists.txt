# Frame OpenGL Test.

add_executable(FrameOpenGLTest
    ../../Asset/Json/DeviceTest.json
    BufferTest.cpp
    BufferTest.h
    DeviceTest.cpp
    DeviceTest.h
    FrameBufferTest.cpp
    FrameBufferTest.h
    LightTest.cpp
    LightTest.h
    main.cpp
    MaterialTest.cpp
    MaterialTest.h
    StaticMeshTest.cpp
    StaticMeshTest.h
    PixelTest.cpp
    PixelTest.h
    ProgramTest.cpp
    ProgramTest.h
    RenderBufferTest.cpp
    RenderBufferTest.h
    RendererTest.cpp
    RendererTest.h
    ShaderTest.cpp
    ShaderTest.h
    TextureTest.cpp
    TextureTest.h
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