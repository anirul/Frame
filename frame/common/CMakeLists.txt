# Sample Common.

add_library(FrameCommon
  STATIC
    application.cpp
    application.h
    draw.cpp
    draw.h
    path_interface.h
)

target_include_directories(FrameCommon
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
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

# Keep FrameCommon's dependency closure local; example/editor targets link
# FRAME_CORE_LIBS explicitly (inside a GNU ld link group on Linux).
set_property(TARGET FrameCommon PROPERTY INTERFACE_LINK_LIBRARIES "")

set_property(TARGET FrameCommon PROPERTY FOLDER "Frame")
