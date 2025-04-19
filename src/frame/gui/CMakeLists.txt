# Frame gui part.

add_library(FrameGui
  STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/draw_gui_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/draw_gui_factory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/gui_logger_sink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/gui_menu_bar_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/gui_window_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_cubemap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_file_dialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_resolution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/window_texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/gui/input_factory.h
    draw_gui_factory.cpp
    input_arcball.cpp
    input_arcball.h
    input_factory.cpp
    input_wasd.cpp
    input_wasd.h
    input_wasd_mouse.cpp
    input_wasd_mouse.h
    window_camera.cpp
    window_cubemap.cpp
    window_file_dialog.cpp
    window_logger.cpp
    window_resolution.cpp
    window_texture.cpp
)

target_include_directories(FrameGui
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
  ${CMAME_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameGui
  PRIVATE
    fmt::fmt
    glm::glm
    imgui::imgui
    protobuf::libprotobuf
)

set_property(TARGET FrameGui PROPERTY FOLDER "Frame")
