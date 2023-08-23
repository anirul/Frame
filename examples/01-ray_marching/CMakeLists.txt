# Sample RayMarching.

add_executable(01_RayMarching
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/ray_marching.json
)

target_include_directories(01_RayMarching
  PUBLIC
    examples/01-ray_marching
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(01_RayMarching
  PUBLIC
    absl::base
    absl::flags
    Frame
    FrameCommon
    FrameFile
    FrameOpenGL
    FrameProto
)

set_property(TARGET 01_RayMarching PROPERTY FOLDER "FrameExamples")
