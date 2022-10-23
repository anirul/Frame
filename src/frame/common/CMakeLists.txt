# Sample Common.

add_library(FrameCommon
  STATIC
    ${CMAKE_SOURCE_DIR}/include/frame/common/application.h
    ${CMAKE_SOURCE_DIR}/include/frame/common/draw.h
    ${CMAKE_SOURCE_DIR}/include/frame/common/path_interface.h
    application.cpp
    draw.cpp
)

target_include_directories(FrameCommon
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameCommon
  PUBLIC
    Frame
    FrameProto
    FrameOpenGL
)

set_property(TARGET FrameCommon PROPERTY FOLDER "Frame")
