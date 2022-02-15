# Sample SceneRendering.

add_executable(SceneRendering
  WIN32
    Application.cpp
    Application.h
    Draw.cpp
    Draw.h
    Input.cpp
    Input.h
    main.cpp
    ${CMAKE_SOURCE_DIR}/Asset/Json/SceneRendering.json
)

target_include_directories(SceneRendering
  PUBLIC
    Sample/SceneRendering
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(SceneRendering
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    spdlog::spdlog
)

set_property(TARGET SceneRendering PROPERTY FOLDER "Sample")
