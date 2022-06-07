# Sample RayMarching.

add_executable(RayMarching
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/ray_marching.json
)

target_include_directories(RayMarching
  PUBLIC
    examples/01-ray_marching
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(RayMarching
  PUBLIC
    Frame
    FrameFile
    FrameOpenGL
    FrameProto
    LibCommon
)

set_property(TARGET RayMarching PROPERTY FOLDER "Examples")
