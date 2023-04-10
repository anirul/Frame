# Image Based Lighting

add_executable(05-ImageBasedLighting
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/image_based_lighting.json
)

target_include_directories(05-ImageBasedLighting
  PUBLIC
    examples/05-image_based_lighting
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(05-ImageBasedLighting
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 05-ImageBasedLighting PROPERTY FOLDER "Examples")
