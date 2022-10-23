# Frame Import.

find_path(HAPPLY_INCLUDE_DIRS "happly.h")

add_library(FrameFile
  STATIC
  	${CMAKE_SOURCE_DIR}/include/frame/file/file_system.h
    file_system.cpp
    image.cpp
    image.h
    image_stb.h
    obj.cpp
    obj.h
    ply.cpp
    ply.h
)

target_include_directories(FrameFile
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE
    ${HAPPLY_INCLUDE_DIRS}
)

target_link_libraries(FrameFile
  PUBLIC
    Frame
    FrameJson
    FrameOpenGL
#   stb::stb
#   tinyobjloader
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")
