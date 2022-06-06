# Sample TextureModif.

add_executable(TextureModif
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/depth_normal.json
)

target_include_directories(TextureModif
  PUBLIC
    examples/03-depth_normal
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

set_property(TARGET TextureModif PROPERTY FOLDER "Examples")
