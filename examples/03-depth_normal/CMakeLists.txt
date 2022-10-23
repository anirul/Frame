# Sample DepthNormal.

add_executable(03_DepthNormal
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/depth_normal.json
)

target_include_directories(03_DepthNormal
  PUBLIC
    examples/03-depth_normal
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(03_DepthNormal
  PUBLIC
    Frame
    FrameCommon
    FrameOpenGL
    FrameProto
)

set_property(TARGET 03_DepthNormal PROPERTY FOLDER "Examples")
