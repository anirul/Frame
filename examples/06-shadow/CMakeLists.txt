# Shadow

add_executable(06-Shadow
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/shadow.json
)

target_include_directories(06-Shadow
  PUBLIC
    examples/06-shadow
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(06-Shadow
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 06-Shadow PROPERTY FOLDER "FrameExamples")
