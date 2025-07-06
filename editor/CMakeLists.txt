# Frame Editor.

add_executable(FrameEditor
  WIN32
    main.cpp
    menubar.cpp
    menubar.h
    menubar_view.cpp
    menubar_view.h
    menubar_file.cpp
    menubar_file.h
    window_start.cpp
    window_start.h
    window_level.cpp
    window_level.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../asset/json/new_project_template.json
)

target_include_directories(FrameEditor
  PUBLIC
    editor/
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameEditor
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameOpenGLFile
    FrameProto
    imgui::imgui
    ImGuiColorTextEdit
)

set_property(TARGET FrameEditor PROPERTY FOLDER "FrameEditor")
