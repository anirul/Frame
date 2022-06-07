# Frame OpenGL.

add_library(FrameOpenGL
  STATIC
    BindInterface.h
    Buffer.cpp
    Buffer.h
    Device.cpp
    Device.h
    FrameBuffer.cpp
    FrameBuffer.h
    Light.cpp
    Light.h
    Material.cpp
    Material.h
    StaticMesh.cpp
    StaticMesh.h
    Pixel.cpp
    Pixel.h
    Program.cpp
    Program.h
    RenderBuffer.cpp
    RenderBuffer.h
    Renderer.cpp
    Renderer.h
    ScopedBind.cpp
    ScopedBind.h
    Shader.cpp
    Shader.h
    Texture.cpp
    Texture.h
    TextureCubeMap.cpp
    TextureCubeMap.h
)

target_include_directories(FrameOpenGL
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGL
  PUBLIC
    Frame
    FrameFile
    FrameOpenGLFile
    FrameProto
    GLEW::GLEW
)

set_property(TARGET FrameOpenGL PROPERTY FOLDER "Frame/OpenGL")

add_subdirectory(Test)
add_subdirectory(File)
