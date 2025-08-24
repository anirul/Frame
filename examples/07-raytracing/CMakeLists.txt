# Ray Tracing

add_executable(07_RayTracing
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/raytracing.json
)

target_include_directories(07_RayTracing
  PUBLIC
    examples/07-raytracing
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(07_RayTracing
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 07_RayTracing PROPERTY FOLDER "FrameExamples")
