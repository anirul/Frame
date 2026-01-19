# Frame gui part.

add_library(FrameGui
  STATIC
    draw_gui_factory.cpp
    draw_gui_factory.h
    draw_gui_interface.h
    gui_logger_sink.h
    gui_menu_bar_interface.h
    gui_window_interface.h
    input_arcball.cpp
    input_arcball.h
    input_factory.cpp
    input_factory.h
    input_wasd.cpp
    input_wasd.h
    input_wasd_mouse.cpp
    input_wasd_mouse.h
    window_camera.cpp
    window_camera.h
    window_cubemap.cpp
    window_cubemap.h
    window_file_dialog.cpp
    window_file_dialog.h
    window_logger.cpp
    window_logger.h
    window_resolution.cpp
    window_resolution.h
    window_texture.cpp
    window_texture.h
    window_json_file.cpp
    window_json_file.h
    window_glsl_file.cpp
    window_glsl_file.h
    window_message_box.cpp
    window_message_box.h
)

target_include_directories(FrameGui
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/ImGuiColorTextEdit
)

target_link_libraries(FrameGui
  PUBLIC
    ImGuiColorTextEdit
  PRIVATE
    glm::glm
    imgui::imgui
    spdlog::spdlog
    protobuf::libprotobuf
    FrameProto
)

set_property(TARGET FrameGui PROPERTY FOLDER "Frame")
