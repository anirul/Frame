# Sample RayMarching.

add_executable(RayMarching
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/Asset/Json/RayMarching.json
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
