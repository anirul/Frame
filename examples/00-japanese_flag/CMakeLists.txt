# Sample JapaneseFlag.

add_executable(00_JapaneseFlag
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/japanese_flag.json
)

target_include_directories(00_JapaneseFlag
  PUBLIC
    examples/00-japanese_flag
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(00_JapaneseFlag
  PUBLIC
    Frame
    FrameCommon
    FrameOpenGL
    FrameProto
)

set_property(TARGET 00_JapaneseFlag PROPERTY FOLDER "Examples")
