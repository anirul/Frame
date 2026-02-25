cmake_minimum_required(VERSION 3.24)

add_executable(02_Cubemap
    WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/cubemap.json
)

target_include_directories(02_Cubemap
  PUBLIC
    examples/02-cubemap
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(02_Cubemap
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    FrameCommon
    ${FRAME_LINK_GROUP_END}
    absl::flags
)

set_target_properties(02_Cubemap PROPERTIES FOLDER "FrameExamples")
