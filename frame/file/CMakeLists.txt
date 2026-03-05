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
)

target_include_directories(FrameFile
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE
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
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")
