# Sample JapaneseFlag.

add_executable(JapaneseFlag
  WIN32
    ../../Asset/Json/JapaneseFlag.Program.json
    ../../Asset/Json/JapaneseFlag.Level.json
    ../../Asset/Json/JapaneseFlag.Material.json
    ../../Asset/Json/JapaneseFlag.Scene.json
    ../../Asset/Json/JapaneseFlag.Texture.json
    Name.h
    main.cpp
)

target_include_directories(JapaneseFlag
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(JapaneseFlag
  PUBLIC
    Sample/JapaneseFlag
)

target_link_libraries(JapaneseFlag
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)
