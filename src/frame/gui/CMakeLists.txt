# Frame gui part.

add_library(FrameGui
  STATIC
  ${CMAKE_SOURCE_DIR}/include/frame/gui/draw_gui_interface.h
  ${CMAKE_SOURCE_DIR}/include/frame/gui/draw_gui_factory.h
  ${CMAKE_SOURCE_DIR}/include/frame/gui/window_resolution.h
  ${CMAKE_SOURCE_DIR}/include/frame/gui/input_wasd_factory.h
  draw_gui_factory.cpp
  input_wasd.cpp
  input_wasd.h
  input_wasd_mouse.cpp
  input_wasd_mouse.h
  input_wasd_factory.cpp
  window_resolution.cpp
)

target_include_directories(FrameGui
  PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAME_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameGui
  PUBLIC
  Frame
  spdlog::spdlog
  glm::glm
  imgui::imgui
)

set_property(TARGET FrameGui PROPERTY FOLDER "Frame")
