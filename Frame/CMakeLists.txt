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
    LevelBase.h
    LevelInterface.h
    LightInterface.h
    Logger.cpp
    Logger.h
    MaterialInterface.h
    ProgramInterface.h
    SceneCamera.cpp
    SceneCamera.h
    SceneLight.cpp
    SceneLight.h
    SceneMatrix.cpp
    SceneMatrix.h
    SceneNodeInterface.h
    SceneTree.cpp
    SceneTree.h
    SceneTreeInterface.h
    SceneStaticMesh.cpp
    SceneStaticMesh.h
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

target_link_libraries(Frame
  PUBLIC
    FrameProto
    SDL2::SDL2
    spdlog::spdlog
)

add_subdirectory(OpenGL)
add_subdirectory(Proto)
add_subdirectory(Test)