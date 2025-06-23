# Frame OpenGL Gui.

add_library(FrameOpenGLGui
  STATIC
    sdl_opengl_draw_gui.cpp
    sdl_opengl_draw_gui.h
)

target_include_directories(FrameOpenGLGui
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGLGui
  PRIVATE
    Frame
    FrameGui
    FrameOpenGL
    GLEW::GLEW
    glm::glm
    imgui::imgui
    protobuf::libprotobuf
    SDL3::SDL3
    spdlog::spdlog
)

set_property(TARGET FrameOpenGLGui PROPERTY FOLDER "Frame/OpenGL")
