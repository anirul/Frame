# Image Based Lighting

add_executable(05_ImageBasedLighting
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/image_based_lighting.json
)

target_include_directories(05_ImageBasedLighting
  PUBLIC
    examples/05-image_based_lighting
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(05_ImageBasedLighting
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 05_ImageBasedLighting PROPERTY FOLDER "FrameExamples")
