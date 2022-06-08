# Frame Proto.

add_library(FrameJson
  STATIC
    ${CMAKE_SOURCE_DIR}/include/frame/json/parse_level.h
    ${CMAKE_SOURCE_DIR}/include/frame/json/proto.h
    parse_material.cpp
    parse_material.h
    parse_pixel.cpp
    parse_pixel.h
    parse_program.cpp
    parse_program.h
    parse_scene_tree.cpp
    parse_scene_tree.h
    parse_texture.cpp
    parse_texture.h
    parse_uniform.cpp
    parse_uniform.h
    parse_level.cpp
)

target_include_directories(FrameJson 
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameJson 
  PUBLIC
    Frame
    FrameProto
    FrameOpenGL
    glm::glm
    protobuf::libprotobuf
)

set_property(TARGET FrameJson PROPERTY FOLDER "Frame")
