# Sample JapaneseFlag.

add_executable(JapaneseFlag
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/japanese_flag.json
)

target_include_directories(JapaneseFlag
  PUBLIC
    examples/00-japanese_flag
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(JapaneseFlag
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)

set_property(TARGET JapaneseFlag PROPERTY FOLDER "Examples")
