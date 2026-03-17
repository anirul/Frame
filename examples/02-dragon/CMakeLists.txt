# Dragon

add_executable(02_Dragon
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/dragon.json
)

target_include_directories(02_Dragon
  PUBLIC
    examples/02-dragon
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(02_Dragon
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    FrameCommon
    ${FRAME_LINK_GROUP_END}
)

set_property(TARGET 02_Dragon PROPERTY FOLDER "FrameExamples")
