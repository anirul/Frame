# Sample SceneRendering.

add_executable(SceneRendering
  WIN32
    ../../Asset/Json/SceneRendering.Program.json
    ../../Asset/Json/SceneRendering.Level.json
    ../../Asset/Json/SceneRendering.Scene.json
    ../../Asset/Json/SceneRendering.Texture.json
    Application.cpp
    Application.h
    Draw.cpp
    Draw.h
    Input.cpp
    Input.h
    main.cpp
)

target_include_directories(SceneRendering
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    Sample/SceneRendering
)

target_link_libraries(SceneRendering
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    spdlog::spdlog
)

set_property(TARGET SceneRendering PROPERTY FOLDER "Sample")
