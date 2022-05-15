# Sample TextureModif.

add_executable(TextureModif
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/Asset/Json/TextureModif.json
)

target_include_directories(TextureModif
  PUBLIC
    Sample/TextureModif
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(TextureModif
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)

set_property(TARGET TextureModif PROPERTY FOLDER "Sample")
