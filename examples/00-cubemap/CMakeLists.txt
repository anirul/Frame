cmake_minimum_required(VERSION 3.24)

add_executable(00_Cubemap
    WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/cubemap.json
)

target_include_directories(00_Cubemap
  PUBLIC
    examples/00-cubemap
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(00_Cubemap
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    FrameCommon
    ${FRAME_LINK_GROUP_END}
    absl::flags
)

set_target_properties(00_Cubemap PROPERTIES FOLDER "FrameExamples")
