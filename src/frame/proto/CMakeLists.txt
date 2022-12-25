# Frame Proto.

add_library(FrameProto
  STATIC
    level.proto
    material.proto
    math.proto
    pixel.proto
    plugin.proto
    program.proto
    scene.proto
    size.proto
    texture.proto
    uniform.proto
)

protobuf_generate(TARGET FrameProto LANGUAGE cpp)

target_link_libraries(FrameProto 
  PUBLIC
    protobuf::libprotoc
    protobuf::libprotobuf
)

set(HEADERS_TO_BE_COPIED
  level.pb.h
  material.pb.h
  math.pb.h
  pixel.pb.h
  plugin.pb.h
  program.pb.h
  scene.pb.h
  size.pb.h
  texture.pb.h
  uniform.pb.h
)

foreach(HEADERN ${HEADERS_TO_BE_COPIED})
  add_custom_command(
    TARGET FrameProto
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_BINARY_DIR}/src/frame/proto/${HEADERN}
      ${CMAKE_SOURCE_DIR}/include/frame/proto
  )
endforeach(HEADERN)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")
