# Sample DepthNormal.

add_executable(DepthNormal
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/depth_normal.json
)

target_include_directories(DepthNormal
  PUBLIC
    examples/03-depth_normal
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DepthNormal
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    LibCommon
)

set_property(TARGET DepthNormal PROPERTY FOLDER "Examples")
