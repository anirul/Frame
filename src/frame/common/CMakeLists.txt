# Sample Common.

add_library(FrameCommon
  OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/common/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/common/draw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/common/path_interface.h
    application.cpp
    draw.cpp
)

target_include_directories(FrameCommon
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameCommon
  PRIVATE
    Frame
    FrameProto
    FrameOpenGL
    FrameVulkan
    absl::flags
    absl::flags_parse
    absl::strings
    glm::glm
    protobuf::libprotobuf
)

set_property(TARGET FrameCommon PROPERTY FOLDER "Frame")
