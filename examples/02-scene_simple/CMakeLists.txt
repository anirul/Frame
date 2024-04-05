# Sample SceneSimple.

add_executable(02_SceneSimple
  WIN32
    main.cpp
    modal_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/scene_simple.json
)

target_include_directories(02_SceneSimple
  PUBLIC
    examples/02-scene_simple
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(02_SceneSimple
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameOpenGLFile
    FrameProto
    imgui::imgui
)

set_property(TARGET 02_SceneSimple PROPERTY FOLDER "FrameExamples")
