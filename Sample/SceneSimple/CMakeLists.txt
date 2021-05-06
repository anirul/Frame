# Sample SceneSimple.

add_executable(SceneSimple
  WIN32
    ../../Asset/Json/SceneSimple.Level.json
    ../../Asset/Json/SceneSimple.Program.json
    ../../Asset/Json/SceneSimple.Scene.json
    ../../Asset/Json/SceneSimple.Texture.json
    Name.h
    main.cpp
)

target_include_directories(SceneSimple
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    Sample/SceneSimple
)

target_link_libraries(SceneSimple
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)
