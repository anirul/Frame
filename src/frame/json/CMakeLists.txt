# Frame Proto.

add_library(FrameJson
  STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/parse_json.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/parse_level.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/parse_pixel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/proto.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/serialize_json.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/json/serialize_level.h
    parse_material.cpp
    parse_material.h
    parse_pixel.cpp
    parse_program.cpp
    parse_program.h
    parse_scene_tree.cpp
    parse_scene_tree.h
    parse_texture.cpp
    parse_texture.h
    parse_uniform.cpp
    parse_uniform.h
    parse_level.cpp
    serialize_material.cpp
    serialize_material.h
    serialize_program.cpp
    serialize_program.h
    serialize_scene_tree.cpp
    serialize_scene_tree.h
    serialize_texture.cpp
    serialize_texture.h
    serialize_level.cpp
    serialize_uniform.cpp
    serialize_uniform.h
)

target_include_directories(FrameJson 
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameJson
  PRIVATE
    Frame
    FrameProto
    FrameOpenGL
    glm::glm
    GLEW::GLEW
    glm::glm
    protobuf::libprotobuf
    spdlog::spdlog
)

set_property(TARGET FrameJson PROPERTY FOLDER "Frame")
