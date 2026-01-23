# Sample JapaneseFlag.

add_executable(00_JapaneseFlag
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/japanese_flag.json
)

target_include_directories(00_JapaneseFlag
  PUBLIC
    examples/00-japanese_flag
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(00_JapaneseFlag
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    ${FRAME_LINK_GROUP_END}
    FrameCommon
)

set_property(TARGET 00_JapaneseFlag PROPERTY FOLDER "FrameExamples")
