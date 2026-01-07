# Ray Tracing (BVH)

add_executable(04_RayTracingBvh
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/raytracing_bvh.json
)

target_include_directories(04_RayTracingBvh
  PUBLIC
    examples/04-raytracing_bvh
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(04_RayTracingBvh
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    ${FRAME_LINK_GROUP_END}
    FrameCommon
)

set_property(TARGET 04_RayTracingBvh PROPERTY FOLDER "FrameExamples")
