# Sample PointCloud.

add_executable(04_PointCloud
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/point_cloud.json
)

target_include_directories(04_PointCloud
  PUBLIC
    examples/04_japanese_flag
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(04_PointCloud
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 04_PointCloud PROPERTY FOLDER "FrameExamples")
