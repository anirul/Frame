# Sample Common.

add_library(FrameCommon
  OBJECT
    application.cpp
    application.h
    draw.cpp
    draw.h
    path_interface.h
)

target_include_directories(FrameCommon
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/frame/proto
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
