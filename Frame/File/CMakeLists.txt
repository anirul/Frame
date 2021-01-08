# Frame Import.

add_library(FrameFile
  STATIC
	LoadImage.cpp
	LoadImage.h
	LoadStaticMesh.cpp
	LoadStaticMesh.h
)

target_include_directories(FrameFile
  PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE 
	${STB_INCLUDE_DIRS}
)

target_link_libraries(FrameFile
  PUBLIC
	Frame
	FrameProto
	FrameOpenGL
	assimp::assimp
)