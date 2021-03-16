# Frame Import.

add_library(FrameOpenGLFile
  STATIC
	LoadProgram.cpp
	LoadProgram.h
	LoadStaticMesh.cpp
	LoadStaticMesh.h
	LoadTexture.cpp
	LoadTexture.h
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
	FrameProto
	FrameOpenGL
)
