# Frame Proto

add_library(FrameProto STATIC
  bvh_cache.proto
  obj_cache.proto
  image_cache.proto
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

# Put generated files in the build tree (NOT in the repo)
set(FRAME_PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/frame/proto")
file(MAKE_DIRECTORY "${FRAME_PROTO_GEN_DIR}")

# Generate .pb.h/.pb.cc into FRAME_PROTO_GEN_DIR
protobuf_generate(
  TARGET FrameProto
  LANGUAGE cpp
  PROTOC_OUT_DIR "${FRAME_PROTO_GEN_DIR}"
)

target_link_libraries(FrameProto
  PRIVATE
    protobuf::libprotobuf
)

# Consumers of FrameProto need the generated headers. Keep the include path one
# level above so "frame/proto/*.pb.h" resolves to the generated tree.
target_include_directories(FrameProto
  PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# Clean target (simple + correct)
add_custom_target(FrameCleanProto
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${FRAME_PROTO_GEN_DIR}"
)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")
set_property(TARGET FrameCleanProto PROPERTY FOLDER "Frame")
