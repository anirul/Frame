# Sample SceneSimple.

add_executable(SceneSimple
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/Asset/Json/SceneSimple.json
)

target_include_directories(SceneSimple
  PUBLIC
    Sample/SceneSimple
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(SceneSimple
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)

set_property(TARGET SceneSimple PROPERTY FOLDER "Sample")
