# Frame Import.

add_library(FrameFile
  STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/file/file_system.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/frame/file/image_stb.h
    file_system.cpp
    image.cpp
    image.h
    obj.cpp
    obj.h
    ply.cpp
    ply.h
)

target_include_directories(FrameFile
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_BINARY_DIR}
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
