# Frame Import.

add_library(FrameOpenGLFile
  STATIC
    load_program.cpp
    load_program.h
    load_mesh.cpp
    load_mesh.h
    load_texture.cpp
    load_texture.h
)

frame_enable_internal_backend_access(FrameOpenGLFile)

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
    FrameWow
    FrameProto
    FrameJson
    glad::glad
    glm::glm
    assimp::assimp
    protobuf::libprotobuf
    spdlog::spdlog
)

set_property(TARGET FrameOpenGLFile PROPERTY FOLDER "Frame/OpenGL")
