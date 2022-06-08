# Frame Proto.

add_library(FrameProto
  STATIC
    math.proto
    pixel.proto
    size.proto
    uniform.proto
    material.proto
    program.proto
    scene.proto
    texture.proto
    level.proto  
)

protobuf_generate(TARGET FrameProto LANGUAGE cpp)

target_link_libraries(FrameProto 
  PUBLIC
    protobuf::libprotoc
    protobuf::libprotobuf
)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")
