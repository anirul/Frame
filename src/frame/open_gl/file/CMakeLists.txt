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
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE 
	${STB_INCLUDE_DIRS}
)

target_link_libraries(FrameOpenGLFile
  PUBLIC
	Frame
	FrameFile
	FrameOpenGL
	FrameProto
)

set_property(TARGET FrameOpenGLFile PROPERTY FOLDER "Frame/OpenGL")
