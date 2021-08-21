# Sample RayMarching.

add_executable(RayMarching
  WIN32
    ../../Asset/Json/RayMarching.Level.json
    ../../Asset/Json/RayMarching.Material.json
    ../../Asset/Json/RayMarching.Program.json
    ../../Asset/Json/RayMarching.Scene.json
    ../../Asset/Json/RayMarching.Texture.json
    Name.h
    main.cpp
)

target_include_directories(RayMarching
  PUBLIC
    Sample/RayMarching
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(RayMarching
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
    SampleCommon
)

set_property(TARGET RayMarching PROPERTY FOLDER "Sample")
