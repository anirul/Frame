# Sample JapaneseFlag.

add_executable(JapaneseFlag
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/Asset/Json/JapaneseFlag.json
)

target_include_directories(JapaneseFlag
  PUBLIC
    Sample/JapaneseFlag
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

set_property(TARGET JapaneseFlag PROPERTY FOLDER "Sample")
