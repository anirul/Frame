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
    SceneCamera.cpp
    SceneCamera.h
    SceneLight.cpp
    SceneLight.h
    SceneMatrix.cpp
    SceneMatrix.h
    SceneNodeInterface.h
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

if(UNIX)
    set(SDL_LIB SDL2)
else()
    set(SDL_LIB SDL2::SDL2)
endif()

target_link_libraries(Frame
  PUBLIC
    FrameProto
    ${SDL_LIB}
    spdlog::spdlog
    glm
)

add_subdirectory(File)
add_subdirectory(OpenGL)
add_subdirectory(Proto)
add_subdirectory(Test)
