# Sample SceneSimple.

add_executable(SceneSimple
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/scene_simple.json
)

target_include_directories(SceneSimple
  PUBLIC
    examples/02-scene_simple
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(SceneSimple
  PUBLIC
    Frame
    FrameOpenGL
    FrameOpenGLFile
    FrameProto
    SampleCommon
)

set_property(TARGET SceneSimple PROPERTY FOLDER "Examples")
