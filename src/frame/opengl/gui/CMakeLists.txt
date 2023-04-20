# Frame OpenGL Gui.

add_library(FrameOpenGLGui
  STATIC
    imgui_impl_opengl3.cpp
    imgui_impl_opengl3.h
    imgui_impl_opengl3_loader.h
    imgui_impl_sdl.cpp
    imgui_impl_sdl.h
    sdl_opengl_draw_gui.cpp
    sdl_opengl_draw_gui.h
)

target_include_directories(FrameOpenGLGui
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGLGui
  PRIVATE
    fmt::fmt
    Frame
    FrameGui
    FrameOpenGL
    GLEW::GLEW
    glm::glm
    imgui::imgui
    protobuf::libprotobuf
    SDL2::SDL2
    spdlog::spdlog
)

set_property(TARGET FrameOpenGLGui PROPERTY FOLDER "Frame/OpenGL")
