# Ray Tracing

add_executable(03_RayTracing
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/raytracing.json
)

target_include_directories(03_RayTracing
  PUBLIC
    examples/03-raytracing
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(03_RayTracing
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    ${FRAME_LINK_GROUP_END}
    FrameCommon
)

set_property(TARGET 03_RayTracing PROPERTY FOLDER "FrameExamples")
