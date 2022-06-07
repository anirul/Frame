# Frame Proto.

add_library(FrameProto
  STATIC
    ${CMAKE_SOURCE_DIR}/include/frame/proto/parse_level.h
    ${CMAKE_SOURCE_DIR}/include/frame/proto/proto.h
    level.proto
    material.proto
    math.proto
    pixel.proto
    program.proto
    scene.proto
    size.proto
    texture.proto
    uniform.proto
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
    proto.cpp
)

target_include_directories(FrameProto 
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src/frame/proto
)

# I still doesn't know how to pass the path correctly to the generate in case
# there is any external include.
protobuf_generate(TARGET FrameProto LANGUAGE cpp)

target_link_libraries(FrameProto 
  PUBLIC
    Frame
    FrameOpenGL
    glm::glm
    protobuf::libprotoc
    protobuf::libprotobuf
)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")
