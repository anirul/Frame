# Frame Import.

add_library(FrameFile
  STATIC
    file_system.cpp
    file_system.h
    image.cpp
    image.h
    image_stb.cpp
    image_stb.h
    image_cache.cpp
    image_cache.h
    obj_cache.cpp
    obj_cache.h
    obj.cpp
    obj.h
    ply.cpp
    ply.h
)

target_include_directories(FrameFile
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/frame/proto
  PRIVATE
    ${HAPPLY_INCLUDE_DIRS}
    ${STB_INCLUDE_DIRS}
)

target_link_libraries(FrameFile
  PUBLIC
    Frame
    FrameJson
    FrameOpenGL
    glm::glm
    protobuf::libprotobuf
    spdlog::spdlog
    tinyobjloader::tinyobjloader
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")
