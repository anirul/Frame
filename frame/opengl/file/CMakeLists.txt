# Frame Import.

add_library(FrameOpenGLFile
  STATIC
    load_program.cpp
    load_program.h
    load_static_mesh.cpp
    load_static_mesh.h
    load_texture.cpp
    load_texture.h
)

target_include_directories(FrameOpenGLFile
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE
    ${STB_INCLUDE_DIRS}
)

target_link_libraries(FrameOpenGLFile
  PRIVATE
    Frame
    FrameFile
    FrameOpenGL
    FrameProto
    FrameJson
    glad::glad
    glm::glm
    protobuf::libprotobuf
    spdlog::spdlog
)

set_property(TARGET FrameOpenGLFile PROPERTY FOLDER "Frame/OpenGL")
