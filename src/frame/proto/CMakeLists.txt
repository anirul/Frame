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
  PRIVATE
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
      ${CMAKE_CURRENT_BINARY_DIR}/${HEADERN}
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/proto
  )
endforeach(HEADERN)

set(PROTO_FILES
    level
    material
    math
    pixel
    plugin
    program
    scene
    size
    texture
    uniform
)

add_custom_target(
    clean_proto
    COMMAND ${CMAKE_COMMAND} -E remove
    foreach(PROTO_FILE ${PROTO_FILES})
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE}.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE}.pb.cc
    endforeach()
)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")
