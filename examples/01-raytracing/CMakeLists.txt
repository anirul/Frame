# Ray Tracing (simple)

add_executable(01_RayTracing
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/raytracing.json
)

target_include_directories(01_RayTracing
  PUBLIC
    examples/01-raytracing
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(01_RayTracing
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    FrameCommon
    ${FRAME_LINK_GROUP_END}
)

set_property(TARGET 01_RayTracing PROPERTY FOLDER "FrameExamples")
