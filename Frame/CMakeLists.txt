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
    EntityId.h
    Error.cpp
    Error.h
    Frame.h
    ImageInterface.h
    InputInterface.h
    Level.cpp
    Level.h
    LevelInterface.h
    LightInterface.h
    Logger.cpp
    Logger.h
    MaterialInterface.h
    ProgramInterface.h
    NameInterface.h
    NodeCamera.cpp
    NodeCamera.h
    NodeLight.cpp
    NodeLight.h
    NodeMatrix.cpp
    NodeMatrix.h
    NodeInterface.h
    NodeStaticMesh.cpp
    NodeStaticMesh.h
    RendererInterface.h
    StaticMeshInterface.h
    TextureInterface.h
    UniformInterface.h
    UniformWrapper.cpp
    UniformWrapper.h
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
    glm::glm
)


set_property(TARGET Frame PROPERTY FOLDER "Frame")

add_subdirectory(File)
add_subdirectory(OpenGL)
add_subdirectory(Proto)
add_subdirectory(Test)
