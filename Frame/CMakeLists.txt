# Frame part.

add_library(Frame
  STATIC
    BindInterface.h
    BufferInterface.h
    Camera.cpp
    Camera.h
    CameraInterface.h
    DeviceInterface.h
    DrawInterface.h
    Error.cpp
    Error.h
    Frame.h
    InputInterface.h
    LevelBase.cpp
    LevelBase.h
    LevelInterface.h
    LightInterface.h
    Logger.cpp
    Logger.h
    MaterialInterface.h
    ProgramInterface.h
    NodeCamera.cpp
    NodeCamera.h
    NodeLight.cpp
    NodeLight.h
    NodeMatrix.cpp
    NodeMatrix.h
    NodeInterface.h
    NodeStaticMesh.cpp
    NodeStaticMesh.h
    StaticMeshInterface.h
    TextureInterface.h
    UniformInterface.h
    Window.cpp
    Window.h
    WindowInterface.h
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Why??? this works and you also need to change:
# > export MESA_GL_VERSION_OVERRIDE=3.3
# on linux for it to run.
if(UNIX)
    set(SDL2_LIBRARY SDL2)
else()
    set(SDL2_LIBRARY SDL2::SDL2)
endif()

target_link_libraries(Frame
  PUBLIC
    FrameProto
    ${SDL2_LIBRARY}
    spdlog::spdlog
    glm
)

add_subdirectory(File)
add_subdirectory(OpenGL)
add_subdirectory(Proto)
add_subdirectory(Test)
